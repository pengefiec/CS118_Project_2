A. DV algorithm implementation<br/>
&nbsp; &nbsp; &nbsp; 	compose update message<br/>
&nbsp; &nbsp; &nbsp; 	process update message<br/>
&nbsp; &nbsp; &nbsp; 	update forwarding table<br/>
&nbsp; &nbsp; &nbsp; 	send update message <br/>
B. packet processing<br/>
&nbsp; &nbsp; &nbsp; 	modify header<br/>
&nbsp; &nbsp; &nbsp; 	replay packet<br/>
C. sender and receiver<br/>
D. Topology discovery<br/><br/>
compile using "g++ -std=c++11 my-router.cpp -pthread -o myrouter"<br/>
run with "./myrouter [router index]" 
each router should be a separated terminal.
