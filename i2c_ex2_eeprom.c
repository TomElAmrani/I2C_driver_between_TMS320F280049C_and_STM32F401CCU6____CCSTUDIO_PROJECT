//#############################################################################
//
// FILE:   i2c_ex2_eeprom.c
//
// TITLE:  I2C DIGI POTENIOEMETER
//
//!
//! \b External \b Connections on Control card\n
//!  - Connect I2C interface to buses
//!  - Connect GPIO10/SDAA on ControlCard to SDA (serial data) line
//!  - Connect GPIO8/SCLA on ControlCard to  SCL (serial clock) line
//!
//
//#############################################################################
//
// Included Files
//
#include "driverlib.h"
#include "device.h"
#include "board.h"

//
// Defines digital Potentiometers Address of MCP45HV51
//

//
// THE FIRST BYTE IN I2C FRAME NAMED CONTROL Byte is : device ADDRESS | 1bit for read/write (write=0)
//

#define DP1_ADDRESS     0x3C
#define DP2_ADDRESS     0x3D
#define DP3_ADDRESS     0x3E
#define DP4_ADDRESS     0x3F


//
// THE SECOND BYTE IN I2C FRAME NAMED Command Byte is : Memory ADDRESS | Command | 2bits reserved
//

//
// MEMORY MAP of the digital potentiometer MCP45HV51
//
typedef enum MemoryAddress{
    WIPER_0_ADDRESS=0x00, //Allowed Commands: Read, Write,Increment, Decrement
    TCON_REG_ADDRESS=0x04 //Allowed Commands: Read, Write
} MemoryAddress;

//
// Commands
//
typedef enum Command{
    WRITE_DATA=0,
    INCREMENT=1,
    DECREMENT=2,
    READ_DATA=3
} Command;

// when increment or decrement a wiper , we need only two bytes (Control Byte and Command byte)
// when we write data we need another byte , which is the data byte
// for TCON register, we need only W and B terminals , we will disconnect A to avoid any current flow
// TCON four writable bits = 0b1011 -> See datasheet section (4.4.1.2 Terminal Control (TCON) Registers)

#define TCON_DATA_CONFIG 0x0B  //sent in initialisation to configure the potentiometer

//the data range for the wiper register is 0x00-0xFF
//0x00 -> gives typically 0 ohm
//0xFF gives typically 5k ohm


#define MAX_FAIL_COUNT 1000 // if the message is not acknowledged 1000 times,
                            // the MCU will stop communicate with it until a reset happens

struct I2C_Msg {
    uint16_t msgStatus;                 // Word stating what state msg is in.
                                        // See MSG_STATUS_* defines above.
    uint8_t targetAddr;                 // Target address tied to the message.
    MemoryAddress memoryAddr;           // memory address of register
    Command command;                    // read , write , decrement , increment
    uint16_t data_out;                  // holding message data to send
    uint16_t data_in;                   // holding message data received
    uint16_t failCount;                 // failing of the message
};


//
// I2C message states for I2CMsg struct
//
#define MSG_STATUS_INACTIVE         0x0000 // Message not in use, do not send
#define MSG_STATUS_SEND_WITHSTOP    0x0010 // Send message with stop bit
#define MSG_STATUS_WRITE_BUSY       0x0011 // Message sent, wait for stop
#define MSG_STATUS_SEND_NOSTOP      0x0020 // Send message without stop bit
#define MSG_STATUS_SEND_NOSTOP_BUSY 0x0021 // Message sent, wait for ARDY
#define MSG_STATUS_RESTART          0x0022 // Ready to become controller-receiver
#define MSG_STATUS_READ_BUSY        0x0023 // Wait for stop before reading data


//
// Error messages for read and write functions
//
#define ERROR_BUS_BUSY              0x1000
#define ERROR_STOP_NOT_READY        0x5555
#define SUCCESS                     0x0000


//
// gloabal variables
//

struct I2C_Msg msg_DP1={MSG_STATUS_SEND_WITHSTOP,
                    DP1_ADDRESS,
                    WIPER_0_ADDRESS,
                    WRITE_DATA,
                    0x57,
                    0x57,
                    0};
struct I2C_Msg msg_DP2={MSG_STATUS_SEND_NOSTOP,
                    DP1_ADDRESS,
                    WIPER_0_ADDRESS,
                    READ_DATA,
                    0x57,
                    0x57,
                    0};
struct I2C_Msg msg_DP3={MSG_STATUS_SEND_WITHSTOP,
                    DP3_ADDRESS,
                    WIPER_0_ADDRESS,
                    WRITE_DATA,
                    0x57,
                    0x57,
                    0};
struct I2C_Msg msg_DP4={MSG_STATUS_SEND_WITHSTOP,
                    DP4_ADDRESS,
                    WIPER_0_ADDRESS,
                    WRITE_DATA,
                    0x57,
                    0x57,
                    0};



uint8_t malFunctioningTargets[4];
volatile uint8_t j=0;


struct I2C_Msg *currentMsgPtr;                   // Used in interrupt



//
// Function Prototypes
//
void initI2C(void);
uint16_t readData(struct I2C_Msg *msg);
uint16_t writeData(struct I2C_Msg *msg);

void fail(uint8_t targetAddress);

__interrupt void i2cAISR(void);

//
// Main
//
void main(void)
{
    uint16_t error;



    //
    // Initialize device clock and peripherals
    //
    Device_init();

    //
    // Disable pin locks and enable internal pullups.
    //
    Device_initGPIO();


    //
    // Initialize PIE and clear PIE registers. Disable CPU interrupts.
    //
    Interrupt_initModule();

    //
    // Initialize the PIE vector table with pointers to the shell Interrupt
    // Service Routines (ISR).
    //
    Interrupt_initVectorTable();

    //
    // Interrupts that are used in this example are re-mapped to ISR functions
    // found within this file.
    //
    Interrupt_register(INT_I2CA, &i2cAISR);

    Board_init();

    //
    // Set I2C use, initializing it for FIFO mode
    //
    initI2C();

    //
    // Clear incoming message buffer
    //
    //for (i = 0; i < MAX_BUFFER_SIZE; i++)
    //{
    //    i2cMsgIn.msgBuffer[i] = 0x0000;
    //}

    //
    // Set message pointer used in interrupt to point to outgoing message
    //
    currentMsgPtr = &msg_DP1;

    //
    // Enable interrupts required for this example
    //
    Interrupt_enable(INT_I2CA);

    //
    // Enable Global Interrupt (INTM) and realtime interrupt (DBGM)
    //
    EINT;
    ERTM;

    //
    // Loop indefinitely
    //
    while(1)
    {
       DEVICE_DELAY_US(1000);
       currentMsgPtr = &msg_DP1;
       currentMsgPtr->msgStatus= MSG_STATUS_SEND_WITHSTOP;
       error = writeData(&msg_DP1);

    }
}

//
// Function to configure I2C A in FIFO mode.
//
void initI2C()
{
    //
    // Must put I2C into reset before configuring it
    //
    I2C_disableModule(I2CA_BASE);

    //
    // I2C configuration. Use a 400kHz I2CCLK with a 33% duty cycle.
    //
    I2C_initController(I2CA_BASE, DEVICE_SYSCLK_FREQ, 100000, I2C_DUTYCYCLE_33);
    I2C_setBitCount(I2CA_BASE, I2C_BITCOUNT_8);
    I2C_setEmulationMode(I2CA_BASE, I2C_EMULATION_FREE_RUN);

    //
    // Enable stop condition and register-access-ready interrupts
    //
    I2C_enableInterrupt(I2CA_BASE, I2C_INT_STOP_CONDITION |
                                   I2C_INT_REG_ACCESS_RDY);

    //
    // FIFO configuration
    //
    I2C_enableFIFO(I2CA_BASE);
    I2C_clearInterruptStatus(I2CA_BASE, I2C_INT_RXFF | I2C_INT_TXFF);

    //
    // Configuration complete. Enable the module.
    //
    I2C_enableModule(I2CA_BASE);
}

//
// Function to send the data that is to be written to the DIGITAL POTENTIOMETER
//
uint16_t writeData(struct I2C_Msg *msg)
{

    I2C_setTargetAddress(I2CA_BASE, msg->targetAddr);

    //
    // Wait until the STP bit is cleared from any previous controller
    // communication. Clearing of this bit by the module is delayed until after
    // the SCD bit is set. If this bit is not checked prior to initiating a new
    // message, the I2C could get confused.
    //
    if(I2C_getStopConditionStatus(I2CA_BASE))
    {
        return(ERROR_STOP_NOT_READY);
    }

    //
    // Setup target address
    //
    I2C_setTargetAddress(I2CA_BASE, msg->targetAddr);

    //
    // Check if bus busy
    //
    if(I2C_isBusBusy(I2CA_BASE))
    {
        return(ERROR_BUS_BUSY);
    }

    //
    // Setup number of bytes to send msgBuffer and address
    //
    if(msg->command == WRITE_DATA){
        I2C_setDataCount(I2CA_BASE, 2);
    }else{
        I2C_setDataCount(I2CA_BASE, 1);
    }

    //
    // Setup data to send
    //
    I2C_putData(I2CA_BASE, (((msg->memoryAddr)<<2)+msg->command)<<2 ); // Command byte

    if(msg->command == WRITE_DATA){
    I2C_putData(I2CA_BASE, msg->data_out); // send data byte
    }

    currentMsgPtr->msgStatus = MSG_STATUS_WRITE_BUSY;

    //
    // Send start as controller transmitter
    //
    I2C_setConfig(I2CA_BASE, I2C_CONTROLLER_SEND_MODE);
    I2C_sendStartCondition(I2CA_BASE);
    I2C_sendStopCondition(I2CA_BASE);

    return(SUCCESS);
}

///////////////////////deal with No-acknowlegement

//
// Function to prepare for the data that is to be read from the digital potentiometer
//
uint16_t readData(struct I2C_Msg *msg)
{
    //
    // Wait until the STP bit is cleared from any previous controller
    // communication. Clearing of this bit by the module is delayed until after
    // the SCD bit is set. If this bit is not checked prior to initiating a new
    // message, the I2C could get confused.
    //
    if(I2C_getStopConditionStatus(I2CA_BASE))
    {
        return(ERROR_STOP_NOT_READY);
    }

    //
    // Setup target address
    //
    I2C_setTargetAddress(I2CA_BASE, msg->targetAddr);

    //
    // If we are in the the address setup phase, send the address without a
    // stop condition.
    //
    if(msg->msgStatus == MSG_STATUS_SEND_NOSTOP)
    {
        //
        // Check if bus busy
        //
        if(I2C_isBusBusy(I2CA_BASE))
        {
            return(ERROR_BUS_BUSY);
        }

        //
        // Send data to setup the potentiometer address and operation
        //
        I2C_setDataCount(I2CA_BASE, 1);
        I2C_putData(I2CA_BASE, (((msg->memoryAddr)<<2)+msg->command)<<2 );
        I2C_setConfig(I2CA_BASE, I2C_CONTROLLER_SEND_MODE);
        msg->msgStatus = MSG_STATUS_SEND_NOSTOP_BUSY;
        I2C_sendStartCondition(I2CA_BASE);
    }
    else if(msg->msgStatus == MSG_STATUS_RESTART)
    {
        //
        // Address setup phase has completed. Now setup how many bytes expected
        // and send restart as controller-receiver.
        //
        DEVICE_DELAY_US(100);
        I2C_setDataCount(I2CA_BASE, 2);
        I2C_setConfig(I2CA_BASE, I2C_CONTROLLER_RECEIVE_MODE);
        msg->msgStatus = MSG_STATUS_READ_BUSY;
        I2C_sendStartCondition(I2CA_BASE);
        I2C_sendStopCondition(I2CA_BASE);
    }else{
        msg->failCount++;
    }

    return(SUCCESS);
}

//configure the ISR to receive data correctly and put it in the data section of my msg

//
// I2C A ISR (non-FIFO)
//
__interrupt void i2cAISR(void)
{
    I2C_InterruptSource intSource;

    //
    // Read interrupt source
    //
    intSource = I2C_getInterruptSource(I2CA_BASE);

    //
    // Interrupt source = stop condition detected
    //
    if(intSource == I2C_INTSRC_STOP_CONDITION)
    {
        //
        // If completed message was writing data, reset msg to inactive state
        //
        if(currentMsgPtr->msgStatus == MSG_STATUS_WRITE_BUSY)
        {
            if((I2C_getStatus(I2CA_BASE) & I2C_STS_NO_ACK) != 0)
            {
                currentMsgPtr->msgStatus = MSG_STATUS_SEND_WITHSTOP;
                currentMsgPtr->failCount++;
                I2C_clearStatus(I2CA_BASE, I2C_STS_NO_ACK);
            }else{
                currentMsgPtr->msgStatus = MSG_STATUS_INACTIVE;
            }
        }
        else
        {
            //
            // If a message receives a NACK during the address setup portion of
            // the DIGI POT read, the code further below included in the register
            // access ready interrupt source code will generate a stop
            // condition. After the stop condition is received (here), set the
            // message status to try again. User may want to limit the number
            // of retries before generating an error.
            //
            if(currentMsgPtr->msgStatus == MSG_STATUS_SEND_NOSTOP_BUSY)
            {
                currentMsgPtr->msgStatus = MSG_STATUS_SEND_NOSTOP;
                currentMsgPtr->failCount++;
            }
            //
            // If completed message was reading DIGI POT registers data, reset message to
            // inactive state and read data from FIFO.
            //
            else if(currentMsgPtr->msgStatus == MSG_STATUS_READ_BUSY)
            {
                currentMsgPtr->msgStatus = MSG_STATUS_INACTIVE;

                //read data
                currentMsgPtr->data_in = (I2C_getData(I2CA_BASE) << 8);
                currentMsgPtr->data_in += I2C_getData(I2CA_BASE);

            }
        }
    }
    //
    // Interrupt source = Register Access Ready
    //
    // This interrupt is used to determine when the DIGI POT address setup
    // portion of the read data communication is complete. Since no stop bit
    // is commanded, this flag tells us when the message has been sent
    // instead of the SCD flag.
    //
    else if(intSource == I2C_INTSRC_REG_ACCESS_RDY)
    {
        //
        // If a NACK is received, clear the NACK bit and command a stop.
        // Otherwise, move on to the read data portion of the communication.
        //
        if((I2C_getStatus(I2CA_BASE) & I2C_STS_NO_ACK) != 0)
        {
            I2C_sendStopCondition(I2CA_BASE);
            I2C_clearStatus(I2CA_BASE, I2C_STS_NO_ACK);
        }
        else if(currentMsgPtr->msgStatus == MSG_STATUS_SEND_NOSTOP_BUSY)
        {
            currentMsgPtr->msgStatus = MSG_STATUS_RESTART;
        }
    }
    else
    {
        //
        // Generate some error from invalid interrupt source
        //
        asm("   ESTOP0");
    }

    if(currentMsgPtr->failCount >= MAX_FAIL_COUNT)
    {
        fail(currentMsgPtr->targetAddr);//ignore messages to the device
    }

    //
    // Issue ACK to enable future group 8 interrupts
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP8);
}


//
// Function to be called if data written does NOT match data read
//
void fail(uint8_t targetAddress)
{
    malFunctioningTargets[j]=targetAddress;
    j++;
}
