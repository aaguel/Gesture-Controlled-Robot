#include "project.h"

// ================= Motor & Servo Definitions =================

#define IN1_Write(x) Pin_IN1_Write(x)
#define IN2_Write(x) Pin_IN2_Write(x)

#define SERVO_MIN     3200
#define SERVO_CENTER  4900
#define SERVO_MAX     6500

#define MOTOR_MAX_DUTY 60

#define MOTOR_STOP      0
#define MOTOR_FORWARD   1
#define MOTOR_BACKWARD  2

volatile uint8 motorState = MOTOR_STOP;

// ================= State for Logging =================

uint16 currentServoPWM = SERVO_CENTER;
int16  currentMotorPWM = 0;
char   lastDirection   = 'S';
uint32 timeStamp       = 0;

// ================= TX Ring Buffer =================

#define LOG_TX_BUF_SIZE 256
static volatile uint16 txHead = 0;
static volatile uint16 txTail = 0;
static char txBuf[LOG_TX_BUF_SIZE];

static inline uint16 tx_next(uint16 i)
{
    return (uint16)((i + 1u) % LOG_TX_BUF_SIZE);
}

static inline uint8 tx_empty(void)
{
    return (txHead == txTail);
}

static void tx_enqueue_char(char c)
{
    uint16 n = tx_next(txHead);
    if (n == txTail) return;   // drop if full
    txBuf[txHead] = c;
    txHead = n;
}

static void tx_enqueue_str(const char *s)
{
    while (*s)
    {
        tx_enqueue_char(*s++);
    }
}

static void tx_enqueue_int(int32 v)
{
    char tmp[12];
    int i = 0;

    if (v == 0)
    {
        tx_enqueue_char('0');
        return;
    }

    if (v < 0)
    {
        tx_enqueue_char('-');
        v = -v;
    }

    while (v && i < 11)
    {
        tmp[i++] = (char)('0' + (v % 10));
        v /= 10;
    }

    while (i--)
    {
        tx_enqueue_char(tmp[i]);
    }
}

// ================= Logging =================

static void queue_log(void)
{
    tx_enqueue_str("t=");
    tx_enqueue_int((int32)timeStamp);

    tx_enqueue_str(",dir=");
    tx_enqueue_char(lastDirection);

    tx_enqueue_str(",servo=");
    tx_enqueue_int(currentServoPWM);

    tx_enqueue_str(",motor=");
    tx_enqueue_int(currentMotorPWM);

    tx_enqueue_str("\r\n");
}

// ================= Servo Helpers =================

void set_servo_center(void)
{
    PWM_Servo_WriteCompare(SERVO_CENTER);
    currentServoPWM = SERVO_CENTER;
}

void set_servo_left(void)
{
    PWM_Servo_WriteCompare(SERVO_MIN);
    currentServoPWM = SERVO_MIN;
}

void set_servo_right(void)
{
    PWM_Servo_WriteCompare(SERVO_MAX);
    currentServoPWM = SERVO_MAX;
}

// ================= Motor Update =================

void motor_update(void)
{
    switch (motorState)
    {
        case MOTOR_FORWARD:
            IN1_Write(1);
            IN2_Write(0);
            PWM_1_WriteCompare(MOTOR_MAX_DUTY);
            PWM_2_WriteCompare(0);
            currentMotorPWM = MOTOR_MAX_DUTY;
            break;

        case MOTOR_BACKWARD:
            IN1_Write(0);
            IN2_Write(1);
            PWM_1_WriteCompare(0);
            PWM_2_WriteCompare(MOTOR_MAX_DUTY);
            currentMotorPWM = -MOTOR_MAX_DUTY;
            break;

        default:
            IN1_Write(0);
            IN2_Write(0);
            PWM_1_WriteCompare(0);
            PWM_2_WriteCompare(0);
            currentMotorPWM = 0;
            break;
    }
}

// ================= TX Service (LOW PRIORITY) =================

static void service_tx(void)
{
    if (tx_empty()) return;

    if (UART_GetTxBufferSize() < (UART_TX_BUFFER_SIZE - 1))
    {
        UART_PutChar(txBuf[txTail]);
        txTail = tx_next(txTail);
    }
}

// ================= Main =================

int main(void)
{
    CyGlobalIntEnable;

    UART_Start();
    PWM_Servo_Start();
    PWM_1_Start();
    PWM_2_Start();

    set_servo_center();
    motorState = MOTOR_STOP;

    for (;;)
    {
        timeStamp++;

        // ---------- RX FIRST (CRITICAL) ----------
        while (UART_GetRxBufferSize() > 0)
        {
            uint8 c = UART_ReadRxData();

            switch (c)
            {
                case 'A':
                    motorState = MOTOR_FORWARD;
                    set_servo_center();
                    lastDirection = 'A';
                    break;

                case 'B':
                    motorState = MOTOR_BACKWARD;
                    set_servo_center();
                    lastDirection = 'B';
                    break;

                case 'C':
                    set_servo_right();
                    motorState = MOTOR_FORWARD;
                    lastDirection = 'C';
                    break;

                case 'D':
                    set_servo_left();
                    motorState = MOTOR_FORWARD;
                    lastDirection = 'D';
                    break;

                case 'S':
                    motorState = MOTOR_STOP;
                    set_servo_center();
                    lastDirection = 'S';
                    break;

                default:
                    break;
            }

            motor_update();   // immediate effect
            queue_log();      // enqueue only
        }

        // ---------- TX LAST (NON-CRITICAL) ----------
        service_tx();
    }
}


