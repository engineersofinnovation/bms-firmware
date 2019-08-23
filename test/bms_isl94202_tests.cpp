
#include "tests.h"

#include "bms.h"

#include <time.h>
#include <stdio.h>

extern BmsConfig bms_conf;
extern BmsStatus bms_status;

extern uint8_t mem[0xAB+1];     // defined in isl94202_hw_stub

// fill RAM and flash with suitable values
static void init_ram()
{
    // Cell voltages
    // voltage = hexvalue * 1.8 * 8 / (4095 * 3)
    // hexvalue = voltage / (1.8 * 8) * 4095 * 3
    *((uint16_t*)&mem[0x90]) = 3.0 / (1.8 * 8) * 4095 * 3;      // Cell 1
    *((uint16_t*)&mem[0x92]) = 3.1 / (1.8 * 8) * 4095 * 3;      // Cell 2
    *((uint16_t*)&mem[0x94]) = 3.2 / (1.8 * 8) * 4095 * 3;      // Cell 3
    *((uint16_t*)&mem[0x96]) = 3.3 / (1.8 * 8) * 4095 * 3;      // Cell 4
    *((uint16_t*)&mem[0x98]) = 3.4 / (1.8 * 8) * 4095 * 3;      // Cell 5
    *((uint16_t*)&mem[0x9A]) = 3.5 / (1.8 * 8) * 4095 * 3;      // Cell 6
    *((uint16_t*)&mem[0x9C]) = 3.6 / (1.8 * 8) * 4095 * 3;      // Cell 7
    *((uint16_t*)&mem[0x9E]) = 3.7 / (1.8 * 8) * 4095 * 3;      // Cell 8

    // Pack voltage
    *((uint16_t*)&mem[0xA6]) = 3.3*8 / (1.8 * 32) * 4095;
}

void test_read_cell_voltages()
{
    init_ram();
    bms_read_voltages(&bms_status);
    TEST_ASSERT_EQUAL_FLOAT(3.0, roundf(bms_status.cell_voltages[0] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.1, roundf(bms_status.cell_voltages[1] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.2, roundf(bms_status.cell_voltages[2] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.3, roundf(bms_status.cell_voltages[3] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.4, roundf(bms_status.cell_voltages[4] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.5, roundf(bms_status.cell_voltages[5] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.6, roundf(bms_status.cell_voltages[6] * 100) / 100);
    TEST_ASSERT_EQUAL_FLOAT(3.7, roundf(bms_status.cell_voltages[7] * 100) / 100);
}

void test_read_pack_voltage()
{
    init_ram();
    bms_read_voltages(&bms_status);
    TEST_ASSERT_EQUAL_FLOAT(3.3*8, roundf(bms_status.pack_voltage * 10) / 10);
}

void test_read_min_max_voltage()
{
    init_ram();
    bms_read_voltages(&bms_status);
    TEST_ASSERT_EQUAL(0, bms_status.id_cell_voltage_min);
    TEST_ASSERT_EQUAL(7, bms_status.id_cell_voltage_max);
}

void test_read_pack_current()
{
    bms_conf.shunt_res_mOhm = 2.0;  // take something different from 1.0

    // charge current, gain 5
    mem[0x82] = 0x01U << 2;     // CHING
    mem[0x85] = 0x01U << 4;     // gain 5
    *((uint16_t*)&mem[0x8E]) = 117.14F / 1.8F * 4095 * 5 * bms_conf.shunt_res_mOhm / 1000;   // ADC reading

    bms_read_current(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL_FLOAT(117.1, roundf(bms_status.pack_current * 10) / 10);

    // discharge current, gain 50
    mem[0x82] = 0x01U << 3;     // DCHING
    mem[0x85] = 0x00U;          // gain 50
    *((uint16_t*)&mem[0x8E]) = 12.14F / 1.8F * 4095 * 50 * bms_conf.shunt_res_mOhm / 1000;   // ADC reading

    bms_read_current(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL_FLOAT(-12.14, roundf(bms_status.pack_current * 100) / 100);

    // low current, gain 500
    mem[0x82] = 0x00U;          // neither CHING nor DCHING
    mem[0x85] = 0x02U << 4;     // gain 500
    *((uint16_t*)&mem[0x8E]) = 1.14 / 1.8 * 4095 * 500 * bms_conf.shunt_res_mOhm / 1000;   // ADC reading

    bms_read_current(&bms_conf, &bms_status);
    TEST_ASSERT_EQUAL_FLOAT(0, roundf(bms_status.pack_current * 100) / 100);
}

void test_read_error_flags()
{
    *((uint16_t*)&mem[0x80]) = 0x01U << 2;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_CELL_UNDERVOLTAGE, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 0;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_CELL_OVERVOLTAGE, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 11;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_SHORT_CIRCUIT, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 10;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_DIS_OVERCURRENT, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 9;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_CHG_OVERCURRENT, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 13;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_OPEN_WIRE, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 5;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_DIS_UNDERTEMP, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 4;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_DIS_OVERTEMP, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 7;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_CHG_UNDERTEMP, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 6;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_CHG_OVERTEMP, bms_status.error_flags);

    *((uint16_t*)&mem[0x80]) = 0x01U << 12;
    bms_read_error_flags(&bms_status);
    TEST_ASSERT_EQUAL_UINT32(1U << BMS_ERR_CELL_FAILURE, bms_status.error_flags);
}

void test_apply_dis_ocp_limits()
{
    float act = 0;

    // see datasheet table 10.4
    bms_conf.shunt_res_mOhm = 2.0;  // take something different from 1.0
    bms_conf.dis_oc_delay_ms = 444;
    uint16_t delay = 444 + (1U << 10);

    // lower than minimum possible setting
    bms_conf.dis_oc_limit  = 1;
    act = bms_apply_dis_ocp(&bms_conf);
    TEST_ASSERT_EQUAL_FLOAT(2, act);    // take lowest possible value
    TEST_ASSERT_EQUAL_HEX16(delay | (0x0 << 12), *((uint16_t*)&mem[0x16]));

    // something in the middle
    bms_conf.dis_oc_limit  = 20;
    act = bms_apply_dis_ocp(&bms_conf);
    TEST_ASSERT_EQUAL_FLOAT(16, act);    // round to next lower value
    TEST_ASSERT_EQUAL_HEX16(delay | (0x4U << 12), *((uint16_t*)&mem[0x16]));

    // higher than maximum possible setting
    bms_conf.dis_oc_limit  = 50;
    act = bms_apply_dis_ocp(&bms_conf);
    TEST_ASSERT_EQUAL_FLOAT(48, act);
    TEST_ASSERT_EQUAL_HEX16(delay | (0x7U << 12), *((uint16_t*)&mem[0x16]));
}

void test_apply_chg_ocp_limits()
{
    float act = 0;

    // see datasheet table 10.5
    bms_conf.shunt_res_mOhm = 2.0;  // take something different from 1.0
    bms_conf.chg_oc_delay_ms = 333;
    uint16_t delay = 333 + (1U << 10);

    // lower than minimum possible setting
    bms_conf.chg_oc_limit  = 0.4;
    act = bms_apply_chg_ocp(&bms_conf);
    TEST_ASSERT_EQUAL_FLOAT(0.5, act);    // take lowest possible value
    TEST_ASSERT_EQUAL_HEX16(delay | (0x0 << 12), *((uint16_t*)&mem[0x18]));

    // something in the middle
    bms_conf.chg_oc_limit  = 5.0;
    act = bms_apply_chg_ocp(&bms_conf);
    TEST_ASSERT_EQUAL_FLOAT(4.0, act);    // round to next lower value
    TEST_ASSERT_EQUAL_HEX16(delay | (0x4U << 12), *((uint16_t*)&mem[0x18]));

    // higher than maximum possible setting
    bms_conf.chg_oc_limit  = 50.0;
    act = bms_apply_chg_ocp(&bms_conf);
    TEST_ASSERT_EQUAL_FLOAT(12.0, act);
    TEST_ASSERT_EQUAL_HEX16(delay | (0x7U << 12), *((uint16_t*)&mem[0x18]));
}

void test_apply_dis_scp_limits()
{
    float act = 0;

    // see datasheet table 10.6
    bms_conf.shunt_res_mOhm = 2.0;  // take something different from 1.0
    bms_conf.dis_sc_delay_us = 222;
    uint16_t delay = 222 + (0U << 10);

    // lower than minimum possible setting
    bms_conf.dis_sc_limit  = 5;
    act = bms_apply_dis_scp(&bms_conf);
    TEST_ASSERT_EQUAL_FLOAT(8, act);    // take lowest possible value
    TEST_ASSERT_EQUAL_HEX16(delay | (0x0 << 12), *((uint16_t*)&mem[0x1A]));

    // something in the middle
    bms_conf.dis_sc_limit  = 40;
    act = bms_apply_dis_scp(&bms_conf);
    TEST_ASSERT_EQUAL_FLOAT(32, act);    // round to next lower value
    TEST_ASSERT_EQUAL_HEX16(delay | (0x4U << 12), *((uint16_t*)&mem[0x1A]));

    // higher than maximum possible setting
    bms_conf.dis_sc_limit  = 150;
    act = bms_apply_dis_scp(&bms_conf);
    TEST_ASSERT_EQUAL_FLOAT(128, act);
    TEST_ASSERT_EQUAL_HEX16(delay | (0x7U << 12), *((uint16_t*)&mem[0x1A]));
}

void test_apply_cell_ov_limits()
{
    bms_conf.cell_ov_limit = 4.251;      // default value
    bms_conf.cell_ov_limit_hyst = 0.101;
    bms_conf.cell_ov_delay_ms = 999;
    uint16_t delay = 999 + (1U << 10);
    bms_apply_cell_ovp(&bms_conf);
    TEST_ASSERT_EQUAL_HEX16(0x1E2A, *((uint16_t*)&mem[0x00]));  // limit voltage
    TEST_ASSERT_EQUAL_HEX16(0x0DD4, *((uint16_t*)&mem[0x02]));  // recovery voltage
    TEST_ASSERT_EQUAL_HEX16(delay, *((uint16_t*)&mem[0x10]));   // delay
}

void test_apply_cell_uv_limits()
{
    bms_conf.cell_uv_limit = 2.7;       // default value
    bms_conf.cell_uv_limit_hyst = 0.3;
    bms_conf.cell_uv_delay_ms = 2222;
    uint16_t delay = 2222/1000 + (2U << 10);
    bms_apply_cell_uvp(&bms_conf);
    TEST_ASSERT_EQUAL_HEX16(0x18FF, *((uint16_t*)&mem[0x04]));
    TEST_ASSERT_EQUAL_HEX16(0x09FF, *((uint16_t*)&mem[0x06]));
    TEST_ASSERT_EQUAL_HEX16(delay, *((uint16_t*)&mem[0x12]));
}

void isl94202_tests()
{
    UNITY_BEGIN();

    RUN_TEST(test_read_cell_voltages);
    RUN_TEST(test_read_pack_voltage);
    RUN_TEST(test_read_min_max_voltage);
    RUN_TEST(test_read_pack_current);
    RUN_TEST(test_read_error_flags);

    RUN_TEST(test_apply_dis_ocp_limits);
    RUN_TEST(test_apply_chg_ocp_limits);
    RUN_TEST(test_apply_dis_scp_limits);

    RUN_TEST(test_apply_cell_ov_limits);
    RUN_TEST(test_apply_cell_uv_limits);

    UNITY_END();
}
