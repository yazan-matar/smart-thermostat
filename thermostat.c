#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "sys/clock.h" /* Library used for etimers in the protothread. */
#include "lib/random.h"



/*
*****************************************************************************    
    Definition of which resources to include to meet memory constraints.
*****************************************************************************
*/


/* Resources macro abilitation*/
#define REST_RES_TOGGLE 1
#define REST_RES_STATUS 1


/* Toggle resources */
#define REST_RES_TOGGLE_HEAT 1
#define REST_RES_TOGGLE_AC 1
#define REST_RES_TOGGLE_VENT 1

/* Status resources. */
#define REST_RES_GET_HEAT 1
#define REST_RES_GET_AC 1
#define REST_RES_GET_VENT 1
#define REST_RES_GET_TEMPERATURE 1


/*
*****************************************************************************    
    Definition of upper and lower bounds for:
        - Boot temperature
        - Room temperature
*****************************************************************************
*/


/* Definition of the maximum and minimum boot temperatures for the room. */
#define MAX_BOOT_TEMPERATURE 30
#define MIN_BOOT_TEMPERATURE 10

/* Definition of the maximum and minimum reachable temperatures for the room. */
#define MAX_ROOM_TEMPERATURE 50
#define MIN_ROOM_TEMPERATURE 0


#include "erbium.h"


#if defined (PLATFORM_HAS_BUTTON)
#include "dev/button-sensor.h"
#endif
#if defined (PLATFORM_HAS_LEDS)
#include "dev/leds.h"
#endif
#if defined (PLATFORM_HAS_LIGHT)
#include "dev/light-sensor.h"
#endif
#if defined (PLATFORM_HAS_BATTERY)
#include "dev/battery-sensor.h"
#endif
#if defined (PLATFORM_HAS_SHT11)
#include "dev/sht11-sensor.h"
#endif
#if defined (PLATFORM_HAS_RADIO)
#include "dev/radio-sensor.h"
#endif



/* For CoAP-specific example: not required for normal RESTful Web service. */
#if WITH_COAP == 3
#include "er-coap-03.h"
#elif WITH_COAP == 7
#include "er-coap-07.h"
#elif WITH_COAP == 12
#include "er-coap-12.h"
#elif WITH_COAP == 13
#include "er-coap-13.h"
#else
#warning "Erbium example without CoAP-specific functionality"
#endif /* CoAP-specific example */


/* Definition of the PRINTF macro */
#define PRINTF(...)


/*
    Definition of constants for:
        - Thermostat Update
        - Temperature Update
*/

#define THERMOSTAT_TIMER (20 * CLOCK_SECOND)
#define TEMPERATURE_OBSERVE (5 * CLOCK_SECOND)


/* Definition of the CoaP Server Contiki Process. */
PROCESS(rest_server_example, "Server COAP");
AUTOSTART_PROCESSES(&rest_server_example);

static uint8_t temperature = 0;
static uint8_t is_ac_on = 0;
static uint8_t is_heating_on = 0;
static uint8_t is_ventilation_on = 0;



/*
*****************************************************************************    
    Definition of Toggle Resources
*****************************************************************************
*/



#if defined (PLATFORM_HAS_LEDS)

    #if REST_RES_TOGGLE

        #if REST_RES_TOGGLE_HEAT


        /*
            Definition of the Heating Toggle Resource.
        */
        RESOURCE(heating, METHOD_POST, "actuators/Heat", "title=\"H\";rt=\"H\"");
        void heating_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
        {
          REST.set_header_content_type(response, REST.type.TEXT_PLAIN);

          if (is_ac_on) {
            REST.set_response_status(response, REST.status.FORBIDDEN);
            is_heating_on = 0;
          } else {
            leds_toggle(LEDS_RED);
            if(is_heating_on)
              is_heating_on = 0;
            else
              is_heating_on = 1;
            REST.set_response_status(response, REST.status.OK);
            process_post(&rest_server_example, PROCESS_EVENT_MSG, NULL);
          }

          REST.set_response_payload(response, (uint8_t *)buffer, snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, 
            is_ac_on ? "\nH: OFF\n" :"\nH: %s\n", is_heating_on ? "OFF=>ON" : "ON=>OFF"));

        }

        #endif

        #if REST_RES_TOGGLE_VENT

        /*
            Definition of the Ventilation Toggle Resource.
        */
        RESOURCE(ventilation, METHOD_POST, "actuators/Vent", "title=\"V\";rt=\"V\"");
        void
        ventilation_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
        {
          REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
          
          leds_toggle(LEDS_GREEN);
          
          if(is_ventilation_on)
            is_ventilation_on = 0;
          else
            is_ventilation_on = 1;
            
          process_post(&rest_server_example, PROCESS_EVENT_MSG, NULL);

          REST.set_response_payload(response, (uint8_t *)buffer, snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "\nV: %s\n", 
            is_ventilation_on ? "OFF=>ON" : "ON=>OFF"));
        }

        #endif

        #if REST_RES_TOGGLE_AC

        /*
            Definition of the Air Conditioning Toggle Resource.
        */
        RESOURCE(air_conditioning, METHOD_POST, "actuators/AC", "title=\"AC\";rt=\"AC\"");
        void air_conditioning_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
        {
          REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
          
          if (is_heating_on) {
            REST.set_response_status(response, REST.status.FORBIDDEN);
            is_ac_on = 0;
          } else {
            leds_toggle(LEDS_BLUE);
            if(is_ac_on)
              is_ac_on = 0;
            else
              is_ac_on = 1;
            REST.set_response_status(response, REST.status.OK);
            process_post(&rest_server_example, PROCESS_EVENT_MSG, NULL);
          }

          REST.set_response_payload(response, (uint8_t *)buffer, snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, 
            is_heating_on ? "\nAC: OFF\n" : "\nAC: %s\n", is_ac_on ? "OFF=>ON" : "ON=>OFF"));
        }

        #endif

    #endif /* REST_RES_TOGGLE */    
#endif /* PLATFORM_HAS_LEDS */

/*
*****************************************************************************    
    Definition of Status Resources
*****************************************************************************
*/

#if REST_RES_STATUS

        /*
            Definition of the Temperature Periodic Resource, which is composed of:
                - a Temperature Handler, which returns the current temperature value of the room.
                - a Periodic Temperature Handler, which returns the current temperature value of the room every 5 seconds.
                
        */
        #if REST_RES_GET_TEMPERATURE
        PERIODIC_RESOURCE(temperature, METHOD_GET, "status/Temp", "title=\"T\";rt=\"T\"", TEMPERATURE_OBSERVE);
        void temperature_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
          
          REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
           
          REST.set_response_payload(response, (uint8_t *)buffer, snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, 
            "\nT: %u°C\n", temperature));

        }

        void temperature_periodic_handler(resource_t *r)
        {
          static char content[30];

          /* Build notification. */
          coap_packet_t notification[1]; /* This way the packet can be treated as pointer as usual. */
          coap_init_message(notification, COAP_TYPE_NON, REST.status.OK, 0 );
          coap_set_payload(notification, content, snprintf(content, sizeof(content), "\nT: %u°C\n", temperature));

          /* Notify the registered observers with the given message type, observe option, and payload. */
          REST.notify_subscribers(r, temperature, notification);
        }

        #endif

        /*
            Definition of the Heating Status Resource.
        */
        #if REST_RES_GET_HEAT
        RESOURCE(heating_status, METHOD_GET, "status/Heat", "title=\"HS\";rt=\"HS\"");
        void heating_status_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
          
          REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
          
          REST.set_response_payload(response, (uint8_t *)buffer, snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "H: %s\n",
                   is_heating_on ? "ON" : "OFF"));
        }

        #endif

        /*
            Definition of the Air Conditioning Status Resource.
        */
        #if REST_RES_GET_AC
        RESOURCE(ac_status, METHOD_GET, "status/AC", "title=\"ACS\";rt=\"ACS\"");
        void ac_status_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
          
          REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
           
          REST.set_response_payload(response, (uint8_t *)buffer, snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "AC: %s\n",
                   is_ac_on ? "ON" : "OFF"));

        }


        #endif

        /*
            Definition of the Ventilation Status Resource.
        */
        #if REST_RES_GET_VENT
        RESOURCE(vent_status, METHOD_GET, "status/Vent", "title=\"VS\";rt=\"VS\"");
        void vent_status_handler(void* request, void* response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset) {
          
          REST.set_header_content_type(response, REST.type.TEXT_PLAIN);
          
          REST.set_response_payload(response, (uint8_t *)buffer, snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "V: %s\n",
                   is_ventilation_on ? "ON" : "OFF"));

        }

        #endif

#endif /* REST_RES_STATUS */




PROCESS_THREAD(rest_server_example, ev, data)
{
    static struct etimer et;

    PROCESS_BEGIN();
    
    PRINTF("Starting COAP Server...\n");

    /* Initialize the REST engine. */
    rest_init_engine();
  

    /*
    *****************************************************************************    
        Sensors and Resources Activation.
    *****************************************************************************
    */

    #if defined (PLATFORM_HAS_LEDS)

        #if REST_RES_TOGGLE

            #if REST_RES_LEDS
              rest_activate_resource(&resource_leds);
            #endif

            #if REST_RES_TOGGLE_HEAT
              rest_activate_resource(&resource_heating);
            #endif  

            #if REST_RES_TOGGLE_VENT
              rest_activate_resource(&resource_ventilation);
            #endif

            #if REST_RES_TOGGLE_AC
              rest_activate_resource(&resource_air_conditioning);
            #endif

        #endif /* REST_RES_TOGGLE */

    #endif /* PLATFORM_HAS_LEDS */



    #if defined (PLATFORM_HAS_SHT11)

    /* Activation of the Temperature and Humidity Sensor */
    SENSORS_ACTIVATE(sht11_sensor); 

        #if REST_RES_STATUS
      
            #if REST_RES_GET_TEMPERATURE
              rest_activate_periodic_resource(&periodic_resource_temperature);
            #endif

            #if REST_RES_GET_HEAT
              rest_activate_resource(&resource_heating_status);
            #endif

            #if REST_RES_GET_AC
              rest_activate_resource(&resource_ac_status);
            #endif

            #if REST_RES_GET_VENT
              rest_activate_resource(&resource_vent_status);
            #endif

        #endif /* REST_RES_STATUS */

    #endif /*PLATFORM_HAS_SHT11*/


    temperature = (random_rand()%21) + 10;


    if(temperature > MAX_BOOT_TEMPERATURE)
      temperature = MAX_BOOT_TEMPERATURE;
    if(temperature < MIN_BOOT_TEMPERATURE)
      temperature = MIN_BOOT_TEMPERATURE;

    /* The timer is set to 20 seconds and then activated  */
    etimer_set(&et, THERMOSTAT_TIMER); 

    while(1) {
      /*
        We have TWO types of Event:
            - PROCESS_EVENT_MSG: an Event triggered when a Toggle Resource is used.
            - PROCESS_EVENT_TIMER: an Event triggered when the timer expires.
      */
      PROCESS_WAIT_EVENT();

      /*
        Behaviour:
          - When a PROCESS_EVENT_MSG Event fires, that means a toggle was triggered, so the timer has to be restarted.
          - When a PROCESS_EVENT_TIMER Event fires, the temperature has to change accordingly to the specific. Then the timer restarts.
      */

      if (ev == PROCESS_EVENT_MSG) {
        etimer_restart(&et);
      } 
      else if (ev == PROCESS_EVENT_TIMER) {
        uint8_t multiplier = is_ventilation_on;
        if (is_heating_on && temperature < (MAX_ROOM_TEMPERATURE - multiplier)) {
          temperature = temperature + (1 + multiplier);
        } else if (is_ac_on && temperature > (MIN_ROOM_TEMPERATURE + multiplier)) {
          temperature = temperature - (1 + multiplier);
        }
        etimer_restart(&et);
      }
      printf("Restarting timer...\n");
      
    }

  PROCESS_END();
}