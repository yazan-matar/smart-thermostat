# Smart Thermostat Simulation
A simulation of a smart thermostat in a house.

## Index

* ### [Prerequisites](#pre)
* ### [Getting Started](#get)
* ### [Run a simulation](#run)







## <a name="pre"></a>Prerequisites
Before anything, we need to download the following softwares:

* [Instant Contiki 2.7](https://sourceforge.net/projects/contiki/files/Instant%20Contiki/Instant%20Contiki%202.7/) VM
* VMWare or VirtualBox to run Instant Contiki

## <a name="get"></a>Getting Started
Now let's setup our environment:

* Run the Instant Contiki VM on VMWare or VirtualBox and log in
  * **_username_**: user 
  * **_password_**: user
* Move the following files:
  * _thermostat.c_ in _"contiki/examples/er-rest-example"_
  * _border_router.c_ in _"contiki/examples/ipv6/rpl-border-router"_
* Install **_node-red_**

  ``` sudo npm install -g --unsafe-perm node-red ```


## <a name="run"></a> Run a simulation
Now we are ready to run a simulation!

* _**Run a Cooja simulation**_  

  ```
  cd contiki/tools/cooja   
  ant run 
  ```
  * Start a new simulation
  * Create a Sky Mote
  
    ``` Motes --> Add Motes --> Sky Mote ```
    * In the first one, load _border_router.c_
    * In the other one(s), load _thermostat.c_
  * Now we have to enable the serial port on the mote:
  
	  ``` Right click on the mote --> Show serial port on Sky x ```  
	  ``` Right click on the mote --> Mote tools for Sky x --> Serial Socket (SERVER) ```
    
    This will open a connection between Cooja and node-red (a window will open)
  
  * Check if the motes are in radio range and start the simulation (check the simulation speed)
  
  * On the terminal, do the following:
  
  ``` cd contiki/examples/ipv6/rpl-border-router ```  
  ``` make connect-router-cooja ```
  
  
