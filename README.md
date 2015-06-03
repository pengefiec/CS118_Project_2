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
each router should be a separated terminal.<br/><br/>
To generate DATA packet, the host should be run with ./host 0 A B, and then input the textphrase.<br/>
To generate ADMIN packet, the host should be run with ./host 1 A, and then type in B,1 in the next line, which means the the link between A and B will be changed to 1.
