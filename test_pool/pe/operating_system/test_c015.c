/** @file
 * Copyright (c) 2023-2024, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include "val/common/include/acs_val.h"
#include "val/common/include/acs_pe.h"
#include "val/sbsa/include/sbsa_acs_pe.h"
#include "val/sbsa/include/sbsa_val_interface.h"
#include "val/sbsa/include/sbsa_acs_mpam.h"

#define TEST_NUM   (ACS_PE_TEST_NUM_BASE  +  15)
#define TEST_RULE  "S_MPAM_PE"
#define TEST_DESC  "Check MPAM PE Requirements            "

static void payload(void)
{
    uint64_t data = 0;
    uint32_t index = val_pe_get_index_mpid(val_pe_get_mpid());

    if (g_sbsa_level < 5) {
        val_set_status(index, RESULT_SKIP(TEST_NUM, 01));
        return;
    }
    /* Check if PEs implements FEAT_MPAM */
    /* ID_AA64PFR0_EL1.MPAM bits[43:40] > 0 or ID_AA64PFR1_EL1.MPAM_frac bits[19:16] > 0
       indicates implementation of MPAM extension */

    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       ID_AA64PFR0_EL1.MPAM = %llx", data, index);

    data = VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       ID_AA64PFR1_EL1.MPAM_frac = %llx",
                                                                       data, index);

    if (!((VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR0_EL1), 40, 43) > 0) ||
        (VAL_EXTRACT_BITS(val_pe_reg_read(ID_AA64PFR1_EL1), 16, 19) > 0))) {
            val_set_status(index, RESULT_SKIP(TEST_NUM, 02));
            return;
    }

    /* check support for minimum of 16 physical partition IDs, MPAMIDR_EL1.PARTID_MAX
       must be >= 16 */
    data = VAL_EXTRACT_BITS(val_mpam_reg_read(MPAMIDR_EL1), 0, 15);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       MPAMIDR_EL1.PARTID_MAX = %llx", data, index);

    if (data < 16) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 01));
        return;
    }

    /* check support for MPAM virtulization support indicated by MPAMIDR_EL1.HAS_HCR bit */
    data = VAL_EXTRACT_BITS(val_mpam_reg_read(MPAMIDR_EL1), 17, 17);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       MPAMIDR_EL1.HAS_HCR = %llx", data, index);
    if (data == 0) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 02));
        return;
    }

    /* check support for minimum of 8 virtual partition IDs,
       MPAMIDR_EL1.VPMR_MAX must be > 0 */
    data = VAL_EXTRACT_BITS(val_mpam_reg_read(MPAMIDR_EL1), 18, 20);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       MPAMIDR_EL1.VPMR_MAX = %llx", data, index);

    if (data < 1) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 03));
        return;
    }

    /* Check support for minimum of 2 performance monitor groups (PMGs),
    MPAMIDR_EL1.PMG_MAX must be >= 1 (value of PMG.MAX 1 means PMG 0 and 1 present */
    data = VAL_EXTRACT_BITS(val_mpam_reg_read(MPAMIDR_EL1), 32, 39);
    val_print_primary_pe(ACS_PRINT_DEBUG, "\n       MPAMIDR_EL1.PMG_MAX = %llx", data, index);
    if (data < 1) {
        val_set_status(index, RESULT_FAIL(TEST_NUM, 04));
        return;
    }
    val_set_status(index, RESULT_PASS(TEST_NUM, 01));
}

uint32_t c015_entry(uint32_t num_pe)
{
    uint32_t status = ACS_STATUS_FAIL;

    status = val_initialize_test(TEST_NUM, TEST_DESC, num_pe);
    /* This check is when user is forcing us to skip this test */
    if (status != ACS_STATUS_SKIP)
        val_run_test_payload(TEST_NUM, num_pe, payload, 0);

    /* get the result from all PE and check for failure */
    status = val_check_for_error(TEST_NUM, num_pe, TEST_RULE);
    val_report_status(0, ACS_END(TEST_NUM), TEST_RULE);

    return status;
}
