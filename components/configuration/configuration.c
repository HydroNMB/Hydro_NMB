#include <stdio.h>
#include "configuration.h"
#include "emmc.h"

bool MainDelayComplete =false;

MainStates currentState = Main_START; 

// Initialization function
void InitVariables(void) {
    for (int i = 0; i < NUM_NODES; i++) {
        TEState[i] = TEStates_START;
        EnrollState[i] = READ_SERIAL_NUMBER;
        enrollStatus[i] = ENROLL_INCOMPLETE;
        Database.NumSetupRecords[i] = 0;
    }

    Database.NumSystemRecords = 0;
}


void startup_process(void)
{
	TEStates TEState[NUM_NODES] = {0};
	StartStates StartState[NUM_NODES] = {0};
	EnrollStates EnrollState[NUM_NODES] = {0};
	EnrollStatus enrollStatus[NUM_NODES] = {0};
	// Index values
	uint8_t pumpIndex[NUM_NODES] = {0};
	uint8_t formulaIndex[NUM_NODES] = {0};
	uint8_t c2dFormulaIndex[NUM_NODES] = {0};
	// Transfer statuses
	TransferC2DSetupStatus transferC2DSetupStatus[NUM_NODES] = { [0 ... NUM_NODES-1] = TRANSFER_C2D_SETUP_COMPLETE };
	TransferC2DFirmwareStatus transferC2DFirmwareStatus[NUM_NODES] = { [0 ... NUM_NODES-1] = TRANSFER_C2D_FIRMWARE_COMPLETE };
	
	// Firmware update counters
	uint16_t fwSectorCounter[NUM_NODES] = {0};
	uint32_t fwByteCounter[NUM_NODES] = {0};
	
	typedef struct {
	    uint32_t status[NUM_NODES];
	    uint8_t percentcomplete[NUM_NODES];
	    char currentversion[NUM_NODES][16];  // adjust string length as needed
	    char newversion[NUM_NODES][16];
	} Firmware;
	
	Firmware firmWare = {
	    .status = { [0 ... NUM_NODES-1] = FIRMWARE_UPDATE_UNKNOWN },
	    .percentcomplete = {0},
	    .currentversion = { [0 ... NUM_NODES-1] = "0" },
	    .newversion = { [0 ... NUM_NODES-1] = "0" }
	};                  
	
	switch (currentState)
	{
		case Main_START:
		{
			InitVariables();           //initialize  TEState, EnrollState, enrollStatus, Database.NumSetupRecords to 14 number
			MainDelayComplete = true;
			currentState = GET_NUM_SYSTEMS;
			break;
		}
		case GET_NUM_SYSTEMS:
		{
			 if (MainDelayComplete == true)
			 {  
				 // Determines how many entries there are in the system table (should be 0 or 1) 
				 if (GetNumberOfSystemRecords(MOUNT_POINT"/System.json") == true)                                 
				 {  
					 //If there are 0 entries in the system table at startup, this state writes the default values to the system table
			        if (NumSystemRecords  == 0)      
			        {
						printf("NumSystemRecords is zero\n");
						currentState = WRITE_DEFAULT_SYSTEM_TO_DATABASE;
			           
			        }
			        else
			        {
						printf("NumSystemRecords is non zero\n");
			            currentState = READ_SYSTEM_FROM_DATABASE;
			        }
			    }
			 }
			 break;					
		}
		case WRITE_DEFAULT_SYSTEM_TO_DATABASE :
		{
			// create a function to write default value to systeam table
			if(Write_default_System_json(MOUNT_POINT"/System.json")== true)
			{
				printf("Default value written in System table file \n");
			}
			else
		    {
				printf("Default value written in System table file \n");
			}
			currentState = GET_NUM_FIRMWARES;
			break;
		}
		case READ_SYSTEM_FROM_DATABASE:
		{
			// function to read system table and print log
			//currentState = GET_NUM_FIRMWARES;
			break;
		}
		case GET_NUM_FIRMWARES:
		{
			// Function to read Frimware table 0 or 1
			/*if (GET_NUM_FIRMWARES==0)
			{
				currentState = WRITE_DEFAULT_FW_TO_DATABASE;
			
			}else
			{
				currentState = GET_NUM_FIRMWARES;
			}	
				*/
			break;
		}
		case WRITE_DEFAULT_FW_TO_DATABASE:
		{
			// if READ_SYSTEM_FROM_DATABASE is zero function to write default value to firmware table
			//currentState =CHECK_FOR_UPDATE_BIN
			break;
		}
		case READ_FIRMWARE_FROM_DATABASE:
		{
			//read data from firmware table
			//currentState =CHECK_FOR_UPDATE_BIN
			break;
		}
		case CHECK_FOR_UPDATE_BIN:
		{
			/*if (File.Exists(fwUpdateFileName))
            {
                //if update.bin exists delete it and write all failures to firmware table before continuing startup
                File.Delete(fwUpdateFileName);
                log.Info("Deleted firmware UPDATE.BIN file at gateway application startup.  Setting firmware status to failure for all nodes.");
                firmWare.status[0] = (uint)FirmwareStatus.FIRMWARE_UPDATE_FAILURE;
                firmWare.percentcomplete[0] = 0;
                firmWare.currentversion[0] = "0"; //added 011524 for REV E
                firmWare.newversion[0] = "0"; //added 011524 for REV E
                firmWare.status[1] = (uint)FirmwareStatus.FIRMWARE_UPDATE_FAILURE;
                firmWare.percentcomplete[1] = 0;
                firmWare.currentversion[1] = "0"; //added 011524 for REV E
                firmWare.newversion[1] = "0"; //added 011524 for REV E
                firmWare.status[2] = (uint)FirmwareStatus.FIRMWARE_UPDATE_FAILURE;
                firmWare.percentcomplete[2] = 0;
                firmWare.currentversion[2] = "0"; //added 011524 for REV E
                firmWare.newversion[2] = "0"; //added 011524 for REV E
                firmWare.status[3] = (uint)FirmwareStatus.FIRMWARE_UPDATE_FAILURE;
                firmWare.percentcomplete[3] = 0;
                firmWare.currentversion[3] = "0"; //added 011524 for REV E
                firmWare.newversion[3] = "0"; //added 011524 for REV E
                currentState = WRITE_FIRMWARE_TO_DATABASE;
            }
            else
            {
                TEWait = false;
                MainDelayComplete = false;
                mainDelayTimer.Interval = 1000;
                mainDelayTimer.Start();
                currentState = READ_TIMEZONENAME_FROM_DATABASE;
            }*/
            break;
		}
		case WRITE_FIRMWARE_TO_DATABASE:
		{
			/*
			write values in firmware table  as below for 14 TE 
			log.Info("Deleted firmware UPDATE.BIN file at gateway application startup.  Setting firmware status to failure for all nodes.");
                firmWare.status[0] = (uint)FirmwareStatus.FIRMWARE_UPDATE_FAILURE;
                firmWare.percentcomplete[0] = 0;
                firmWare.currentversion[0] = "0"; //added 011524 for REV E
                firmWare.newversion[0] = "0"; //added 011524 for REV E
			 currentState = READ_TIMEZONENAME_FROM_DATABASE;
			 */
		}
		case READ_TIMEZONENAME_FROM_DATABASE:
		{
			/// Reads the timezone name from the timezone data table.
			//currentState = VALIDATE_SETUP_HYC
		}
		case VALIDATE_SETUP_HYC:
		{
			 //This method sets ack_status = 2 if any entry has it set to 1 or 4 (run this at software start).
			 //currentState = INITIALIZE_LAST_MAX_HYC_APPEND_NUMBER
		}
		case INITIALIZE_LAST_MAX_HYC_APPEND_NUMBER:
		{
			/// This method is ran on start-up of the gateway application.  For tracking purposes, it initializes the last timestamp to 0 if no entries are present in the
            /// xdd_hyc_setup table or reads the last timestamp for the most recent entry.
			/*
			currentState = RUN_TE_SM
			TEWait = false;
            MainDelayComplete = false;*/
            break;
		}
		case RUN_TE_SM:
		{
			/*
			if (!TEWait)
            {
                TEWait = RunSMForTE(CurrentNodeIndex);
            }
            if (MainDelayComplete == true)  //this occurs when the x second timer expires
            {
                if ((TEState[CurrentNodeIndex] == TEStates.TRANSFER_SETUP) && (transferSetupStatus[CurrentNodeIndex] == TransferSetupStatus.TRANSFER_SETUP_INCOMPLETE))
                {
                    TEState[CurrentNodeIndex] = TEStates.POLL;
                    pollStatus[CurrentNodeIndex] = PollStatus.POLL_INCOMPLETE;
                    TEInterface.TEValues[CurrentNodeIndex].UartErrorCounter = 0;
                    Database.DBErrorCounter = 0;
                }
                MainDelayComplete = false;
                mainDelayTimer.Interval = 500;
                mainDelayTimer.Start();
                MainState = MainStates.NEXT_NODE;
            }
			*/
			break;
		}
		case NEXT_NODE:
		{
			/*
			CurrentNodeIndex += 1;
			if (CurrentNodeIndex == 14)
	            {
	                CurrentNodeIndex = 0;
			currentState = CHECK_FOR_NEW_HC_FIRMWARE_FILE;
			
			*/
		}
		case CHECK_FOR_NEW_HC_FIRMWARE_FILE:
		{
			/*
			//this checks to see if a firmware update file exists and get its length
            //if it is found, move it to PROCESS.BIN
			*/
		}
		
		default:
    	break;
	}
	
}