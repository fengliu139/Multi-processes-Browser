#include "wrapper.h"
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <webkit/webkit.h>

#define MAX_TAB 100
int available_tab[MAX_TAB];
extern int errno;

/*
 * Name:		uri_entered_cb
 * Input arguments:'entry'-address bar where the url was entered
 *			 'data'-auxiliary data sent along with the event
 * Output arguments:void
 * Function:	When the user hits the enter after entering the url
 *			in the address bar, 'activate' event is generated
 *			for the Widget Entry, for which 'uri_entered_cb'
 *			callback is called. Controller-tab captures this event
 *			and sends the browsing request to the router(/parent)
 *			process.
 */
void uri_entered_cb(GtkWidget* entry, gpointer data)    //send selector index and uri and type
{
	if(data == NULL)
	{	
		return;
	}

	browser_window* b_window = (browser_window*)data;
	comm_channel channel = b_window->channel;
	
	// Get the tab index where the URL is to be rendered
	int tab_index = query_tab_id_for_request(entry, data);  // this index is what we typed in the selector

	if(tab_index < 0)
	{
		printf("error\n");
                return;
	}
	if(tab_index>=99)
	{
		char alert_message[]="Please enter smaller than 99";
		alert((gchar*)alert_message);
	}
	// Get the URL.
	else
	{
	char* uri = get_entered_uri(entry);
	child_req_to_parent new_req;
	child_request ch_rq;
	req_type rq_tp;
	new_uri_req new_uri;
	
	strcpy(new_uri.uri,uri);
	new_uri.render_in_tab=tab_index;
	ch_rq.uri_req=new_uri;
	rq_tp=NEW_URI_ENTERED; 
	new_req.type=rq_tp;	
	new_req.req=ch_rq;

	if(write(channel.child_to_parent_fd[1], &new_req, sizeof(child_req_to_parent)) == -1)
	{
		perror("fail to send uri information");
	}
	}
}

//This function is to check how the first free index!

int check_free_tab_index(void)
{
    int j, index = -10;
    for (j = 0; j <MAX_TAB; j++) {
        if (available_tab[j] == 0) {
            index = j;
            break;
        }
    }
    return index;
}


/*
 * Name:		new_tab_created_cb
 * Input arguments:	'button' - whose click generated this callback
 *			'data' - auxillary data passed along for handling
 *			this event.
 * Output arguments:    void
 * Function:		This is the callback function for the 'create_new_tab'
 *			event which is generated when the user clicks the '+'
 *			button in the controller-tab. The controller-tab
 *			redirects the request to the parent (/router) process
 *			which then creates a new child process for creating
 *			and managing this new tab.
 */ 
void new_tab_created_cb(GtkButton *button, gpointer data)   //send tab index and type
{	
	if(data == NULL)
	{
		return;
	}

 	int tab_index = ((browser_window*)data)->tab_index;
	
	//This channel have pipes to communicate with router. 
	comm_channel channel = ((browser_window*)data)->channel;


	child_req_to_parent new_req;
	/*We assign values to the sub-struct of new_req*/
	child_request ch_rq;
	req_type r_tp;
	create_new_tab_req crt_tb;

	crt_tb.tab_index= tab_index;
	
	ch_rq.new_tab_req=crt_tb;

	r_tp=CREATE_TAB;
	
	new_req.type=r_tp;
	new_req.req=ch_rq;
	/*We assign values to the sub-struct of new_req*/
	// Users press + button on the control window. 

    	if(write(channel.child_to_parent_fd[1], &new_req, sizeof(new_req)) == -1)
	{
		perror("fail to send data to router to create new tab!!");
	}
	
}

/*
 * Name:                run_control
 * Input arguments:     'comm_channel': Includes pipes to communctaion with Router process
 * Output arguments:    void
 * Function:            This function will make a CONTROLLER window and be blocked until the program terminate.
 */
int run_control(comm_channel comm)
{
	browser_window * b_window = NULL;

	//Create controler process
	create_browser(CONTROLLER_TAB, 0, G_CALLBACK(new_tab_created_cb), G_CALLBACK(uri_entered_cb), &b_window, comm);

	//go into infinite loop.
	show_browser();
	return 0;
}

/*
* Name:                 run_url_browser
* Input arguments:      'nTabIndex': URL-RENDERING tab index
                        'comm_channel': Includes pipes to communctaion with Router process
* Output arguments:     void
* Function:             This function will make a URL-RENDRERING tab Note.
*                       You need to use below functions to handle tab event. 
*                       1. process_all_gtk_events();
*                       2. process_single_gtk_event();
*                       3. render_web_page_in_tab(uri, b_window);
*                       For more details please Appendix B.
*/
int run_url_browser(int nTabIndex, comm_channel comm)
{	
	int i;
	browser_window * b_window = NULL;
	
	//Create controler window
	create_browser(URL_RENDERING_TAB, nTabIndex, G_CALLBACK(new_tab_created_cb), G_CALLBACK(uri_entered_cb), &b_window, comm);

	child_req_to_parent req;
        while (1) 
	{	
		i = read(comm.parent_to_child_fd[0], &req, sizeof(child_req_to_parent));
                if(i>0)
		{
                      	if(req.type == CREATE_TAB) 
			{
				
                       	}
			else if(req.type == NEW_URI_ENTERED)
			{
                                render_web_page_in_tab(req.req.uri_req.uri, b_window);
			}
			else if(req.type == TAB_KILLED)
			{
                        	process_all_gtk_events();
                        	exit(0);
				
			}
			
                      	 else
			{
				fprintf(stderr,"Invalid Command!");
			}
                        
                
		}
		
		else if( i == 0)
		{	
                        process_all_gtk_events();
                        exit(1);
                	
		}
		process_single_gtk_event();
        }
	//--------------------------------------------------------------

	return 0;
}

//----------------------------------------------------------

/*
  This function is to make a pipeb between router and controller, 
  and we let the index as 0!
*/

int make_pipe(comm_channel *ps)
{
	if((pipe(ps[0].parent_to_child_fd))==-1)
	{
		perror("Failed to create parent to child pipe");
		return -1;
	}
	if((pipe(ps[0].child_to_parent_fd))==-1)
	{
		perror("Failed to create child to parent pipe");
		return -1;
	}
	return 0;
}
//--------------------------------------------------------------

int main()
{	int i,k,j;
	child_req_to_parent buffer;
	pid_t pid;
	pid_t child_pid[MAX_TAB];
	comm_channel comm[MAX_TAB];
	for (k=0;k<MAX_TAB;k++)
	{
	   available_tab[k] = 0; // 0 means the tab is available
	}
	make_pipe(&comm[0]);
	pid = fork();
	//-----------The controller!
	if (pid == 0)
	{	
		//Close 'read'
		close(comm[0].child_to_parent_fd[0]);//Close c-p read
		close(comm[0].parent_to_child_fd[1]);//Close p-c write
		run_control(comm[0]);
		//close the pipe after blocking
		close(comm[0].child_to_parent_fd[1]);
		close(comm[0].parent_to_child_fd[0]);
	}

	//--------The router!
	else if(pid > 0)
	{	
		
		int flag = 0;
		//Close 'write'
		close(comm[0].parent_to_child_fd[0]);	//Close p-c read
		close(comm[0].child_to_parent_fd[1]);	//Close c-p write 
		//non-blocking the read
		
		int flags = fcntl(comm[0].child_to_parent_fd[0], F_GETFL, 0);
				
		if (flags == -1) perror("fail to get pipe flags");
		flags |= O_NONBLOCK;
		if (fcntl(comm[0].child_to_parent_fd[0], F_SETFL, flags) == -1)
		    perror("fail to set nonblocking");
		
		
		
		
		int tab_number,select_tab;
		child_req_to_parent kill_buffer;
		//-------------go into the while loop------------------------------------
		while (1)
		{		
			i = read(comm[0].child_to_parent_fd[0], &buffer, sizeof(buffer));
			if (i == -1 && errno != EAGAIN) {
                        perror("fail to read from controller to router pipe");}
			
			else if(i>0)
		    {	int tab_num,tab_int;
			//---------------------------type 1: CREATE_TAB---------------------
			if(buffer.type == CREATE_TAB)
			{
				
				if(flag ==0)					//Initialization
				{
				tab_num=buffer.req.new_tab_req.tab_index;
				tab_int=buffer.req.uri_req.render_in_tab;
				flag++;}
				tab_num++;
				available_tab[tab_num-1] =1;   //1 means the tab with this index is open.
				pipe(comm[tab_num].parent_to_child_fd); //Create pipes
				//------------------set non-blocking------------------------
					flags = fcntl(comm[tab_num].parent_to_child_fd[0], F_GETFL, 0);
		                        if (flags == -1) 
		                            perror("fail to read");
		                        flags |= O_NONBLOCK;
		                        if (fcntl(comm[tab_num].parent_to_child_fd[0], F_SETFL, flags) == -1)
		                            perror("fail to set nonblocking");
				//--------------
					pipe(comm[tab_num].child_to_parent_fd);
					flags = fcntl(comm[tab_num].child_to_parent_fd[0], F_GETFL, 0);
		                        if (flags == -1) 
		                            perror("fail to read");
		                        flags |= O_NONBLOCK;
		                        if (fcntl(comm[tab_num].child_to_parent_fd[0], F_SETFL, flags) == -1)
		                            perror("fail to set nonblocking");
				//-----
				child_pid[tab_num]=fork();
				//-----create tab 
				if(child_pid[tab_num]==0)	
				{
				close(comm[tab_num].child_to_parent_fd[0]);
				close(comm[tab_num].parent_to_child_fd[1]);
				run_url_browser(tab_num, comm[tab_num]);
				}
				else if (child_pid[tab_num] == -1) 
				{
		                   perror("Fail to fork");
		                }
			}
			//--------------------type 2:NEW_URI_ENTERED-----------------
			else if(buffer.type == NEW_URI_ENTERED)
			{
				select_tab = buffer.req.uri_req.render_in_tab;
				
				if (select_tab > tab_num)
				{
		                 fprintf(stderr, "You didn't create this tab!!!\n");
				}
				
				else if(select_tab <= tab_num)
				{
		                write(comm[select_tab].parent_to_child_fd[1], &buffer,sizeof(buffer));
		                }
				else
				{
				fprintf(stderr,"please type valid index!");
				}	
			}
			//----------------------type 3:TAB_KILLED-------------------
			else if(buffer.type == TAB_KILLED)
			{	
				
				//int controller = 0;
				kill_buffer.type = TAB_KILLED;				
				
                                for (i = 1; i <= MAX_TAB; i++) 
				{
                                        if (available_tab[i] == 1)
                                        write(comm[i].parent_to_child_fd[1],&kill_buffer,sizeof(kill_buffer));
					close(comm[i].parent_to_child_fd[1]);  //close pipe after tab terminates
					close(comm[i].child_to_parent_fd[0]); 
					
                                }
				wait(NULL);
                                exit(0);
			}
			//---------------------------------------------------------------------

		
               while(j>0)
		{ 
                        if((j = read(comm[tab_num].child_to_parent_fd[0], &buffer,sizeof(buffer)))!=-1)

                    	{
			if (buffer.type == TAB_KILLED) 
			{
                                
                                close(comm[tab_num].child_to_parent_fd[1]); 
				close(comm[tab_num].parent_to_child_fd[0]);
				tab_num--;
			}	
                        }
                }  

                usleep(1000);
			
		    }
		}


	}

	return 0;
}
