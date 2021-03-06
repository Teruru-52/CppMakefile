#include "gyro.h"

uint8_t Gyro::read_byte(uint8_t reg)
{
    uint8_t rx_data[2];
    uint8_t tx_data[2];

    tx_data[0] = reg | 0x80;
    tx_data[1] = 0x00; // dummy

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, RESET);
    HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 2, 1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, SET);

    return rx_data[1];
}

void Gyro::write_byte(uint8_t reg, uint8_t data)
{
    uint8_t rx_data[2];
    uint8_t tx_data[2];

    tx_data[0] = reg & 0x7F;
    //   tx_data[0] = reg | 0x00;
    tx_data[1] = data; // write data

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, RESET); // CSピン立ち下げ
    HAL_SPI_TransmitReceive(&hspi1, tx_data, rx_data, 2, 1);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, SET); // CSピン立ち上げ
}

void Gyro::IIRInit()
{
    for (int i = 0; i < 4; i++)
    {
        gz_y_pre[i] = 0;
        gz_x_pre[i] = 0;
    }
}

void Gyro::GyroInit()
{
    // turn on LED
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);

    uint8_t who_am_i;

    HAL_Delay(100);                          // wait start up
    who_am_i = read_byte(WHO_AM_I);          // read who am i
    printf("who_am_i = 0x%x\r\n", who_am_i); // check who am i value
    HAL_Delay(10);
    while (flag_gyro == false)
    {
        who_am_i = read_byte(WHO_AM_I);
        if (who_am_i == 0x70)
        {
            flag_gyro = true;
            printf("who_am_i = 0x%x\r\n", who_am_i);
        }
        // error check
        else
        {
            // printf("who_am_i = 0x%x\r\n", who_am_i);
            HAL_Delay(10);
            // printf("gyro_error \r\n");
        }
    }

    HAL_Delay(50);
    write_byte(PWR_MGMT_1, 0x00); // set pwr_might (20MHz)
    HAL_Delay(50);
    write_byte(CONFIG, 0x00); // set config (FSYNCはNC)
    HAL_Delay(50);
    write_byte(GYRO_CONFIG, 0x18); // set gyro config (2000dps)
    HAL_Delay(50);

    // Speaker
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 10);
    HAL_Delay(50);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
}

void Gyro::GyroOffsetCalc()
{
    int16_t gz_raw;
    float gz_nonfil;
    float gz_sum = 0;
    for (int i = 0; i < 1000; i++)
    {
        // H:8bit shift, Link h and l
        gz_raw = (int16_t)((uint16_t)(read_byte(GYRO_ZOUT_H) << 8) | (uint16_t)read_byte(GYRO_ZOUT_L));
        // printf("%d\r\n", gz_raw);
        gz_nonfil = (float)(gz_raw / GYRO_FACTOR); // dps to deg/sec

        gz_sum += gz_nonfil;
        HAL_Delay(1);
    }
    gz_offset = gz_sum / 1000.0;
    // printf("%f\r\n", gyro_offset);

    // turn off LED
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);

    IIRInit();
}

void Gyro::GetGyroData()
{
    int16_t gz_raw;
    float gz_nonfil;

    // H:8bit shift, Link h and l
    gz_raw = (int16_t)((uint16_t)(read_byte(GYRO_ZOUT_H) << 8) | (uint16_t)read_byte(GYRO_ZOUT_L));
    // printf("%d\r\n", gz_raw);
    gz_nonfil = (float)(gz_raw / GYRO_FACTOR) - gz_offset; // dps to deg/sec

    float filtered_gyro_z = b0 * gz_nonfil + b1 * gz_x_pre[0] + b2 * gz_x_pre[1] + a1 * gz_y_pre[0] + a2 * gz_y_pre[1];

    // Shift IIR filter state
    for (int i = 1; i > 0; i--)
    {
        gz_x_pre[i] = gz_x_pre[i - 1];
        gz_y_pre[i] = gz_y_pre[i - 1];
    }
    gz_x_pre[0] = gz_nonfil;
    gz_y_pre[0] = filtered_gyro_z;

    gz = filtered_gyro_z; // IIR filter
    // gz = gz_nonfil; // no filter
    yaw += gz * 0.001;
}