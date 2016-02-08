



Purpose
    To implement a multi-process web-browser.
    Router is the parents process, and it forks Controller and URI_Rendering process.
    Router communicates with Controller and URI_R using pipes.


Compile and execute the program
	(1) load software:
		setenv PATH /soft/gcc-4.7.2/ubuntuamd2010/bin/:$PATH
	(2) compile and execute
		% make clean
		% make
		%./browser


What my program does?
	When user execute ./browser, the Router will fork the Controller, then if you click '+' on the Controller window, it will send message to the Router to fork a URI window. 
	When user type correct URL and the tab number in, the corresponding URI page will render the webpage.
	When the user clicks the "x" button on a url-rendering window, the url-rendering process sends a notification to the router and terminates and close pipe.
    	When the user clicks the "x" button on the controller, the router receives a notification and kills all the tabs and close pipes.


Assumptions
    Tab numbers range from 1 to 99 without reusing any closed tab number.


Error Handling
    (1) If user type a number bigger than 100,there will be an alert message.
    (2) If user type a number but the correasponding tab has not be created, there will be error message.
    (3) Upon the controller being killed, the router send TAB_KILLED request to each living tab, and tabs terminate.
    (4) Before any process terminates, close its pipe-ends file descriptors.

