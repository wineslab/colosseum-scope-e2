/*******************************************************************************
################################################################################
#   Copyright (c) [2017-2019] [Radisys]                                        #
#                                                                              #
#   Licensed under the Apache License, Version 2.0 (the "License");            #
#   you may not use this file except in compliance with the License.           #
#   You may obtain a copy of the License at                                    #
#                                                                              #
#       http://www.apache.org/licenses/LICENSE-2.0                             #
#                                                                              #
#   Unless required by applicable law or agreed to in writing, software        #
#   distributed under the License is distributed on an "AS IS" BASIS,          #
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   #
#   See the License for the specific language governing permissions and        #
#   limitations under the License.                                             #
################################################################################
*******************************************************************************/
#include "common_def.h"
#include "lrg.h"
#include "lkw.x"
#include "lrg.x"
#include "legtp.h"
#include "du_app_mac_inf.h"
#include "du_app_rlc_inf.h"
#include "du_cfg.h"
#include "du_mgr.h"
#include "du_mgr_main.h"
#include "du_utils.h"
#include "GlobalE2node-gNB-ID.h"
#include<ProtocolIE-FieldE2.h>
#include "E2AP-PDU.h"
#include "du_e2ap_msg_hdl.h"
#include "odu_common_codec.h"

#include "bs_connector.h"
#include <string.h>

/* Global variable */
DuCfgParams duCfgParam;
/*******************************************************************
 *
 * @brief Builds Global gNodeB Params
 *
 * @details
 *
 *    Function : BuildGlobalgNBId
 *
 *    Functionality: Building the Plmn and gNB id
 *
 * @params[in] GlobalE2node_gNB_ID_t *gNbId
 * @return ROK     - success
 *         RFAILED - failure
 *
 ******************************************************************/

uint8_t buildGnbId(BIT_STRING_t *id, uint8_t unusedBits, uint8_t byteSize, uint8_t val);

uint8_t BuildGlobalgNBId(GlobalE2node_gNB_ID_t *gNbId)
{
   uint8_t unused = 0;
   uint8_t byteSize = 4;
   uint8_t val = 1;
   uint8_t ret = ROK;

   /* Allocate Buffer size */
   gNbId->global_gNB_ID.plmn_id.size = 3 * sizeof(uint8_t);
   gNbId->global_gNB_ID.plmn_id.buf = NULLP;
   DU_ALLOC(gNbId->global_gNB_ID.plmn_id.buf , gNbId->global_gNB_ID.plmn_id.size);
   if(gNbId->global_gNB_ID.plmn_id.buf == NULLP)
   {
      DU_LOG("\nE2AP: Memory allocation failed for Plmn buffer");
      ret = RFAILED;
   }
   else
   {
      buildPlmnId(duCfgParam.srvdCellLst[0].duCellInfo.cellInfo.nrCgi.plmn, \
         &gNbId->global_gNB_ID.plmn_id);
      /* fill gND Id */
      gNbId->global_gNB_ID.gnb_id.present = GNB_ID_Choice_PR_gnb_ID;
      /* Allocate Buffer size */
      gNbId->global_gNB_ID.gnb_id.choice.gnb_ID.size = byteSize * sizeof(uint8_t);
      gNbId->global_gNB_ID.gnb_id.choice.gnb_ID.buf = NULLP;
      DU_ALLOC(gNbId->global_gNB_ID.gnb_id.choice.gnb_ID.buf, \
         gNbId->global_gNB_ID.gnb_id.choice.gnb_ID.size);
      if(gNbId->global_gNB_ID.gnb_id.choice.gnb_ID.buf == NULLP)
      {
         DU_LOG("\nE2AP: Memory allocation failed for gnb buffer");
         ret = RFAILED;
      }
      else
      {
         //fillBitString(&gNbId->global_gNB_ID.gnb_id.choice.gnb_ID, unused, byteSize, val);
	 buildGnbId(&gNbId->global_gNB_ID.gnb_id.choice.gnb_ID, unused, byteSize, val);
      }
   }
   return ret;
}

// build gNB ID based on IP address
uint8_t buildGnbId(BIT_STRING_t *id, uint8_t unusedBits, uint8_t byteSize, uint8_t val) {

  if(id->buf == NULLP) {
    return RFAILED;
  }

  // get node IP address
  char du_ip_v4_addr[NI_MAXHOST];
  getPrimaryIp(du_ip_v4_addr);

  // get last part of IP
  // E.g., if IP is 10.0.2.23 get 23
  char* delimiter = ".";
  char* ip_last = strtok (du_ip_v4_addr, delimiter);
  for (int i = 0; i < 3; ++i) {
    if (ip_last != NULL)
      ip_last = strtok (NULL, delimiter);
  }

  // insert last part of IP padded with 0
  // E.g., if last part of IP is 23, insert 023
  for (int buf_idx = (byteSize - 2), ip_idx = (strlen(ip_last) - 1); buf_idx >= 0; --buf_idx, --ip_idx) {
    if (ip_idx >= 0)
      id->buf[buf_idx] = (int) (ip_last[ip_idx] - '0');
    else
      id->buf[buf_idx] = 0;
  }

  id->buf[byteSize-1] = val;
  id->bits_unused = unusedBits;

  printf("\ngnb-ID: ");
  for (int i = 0; i < byteSize; ++i) {
    printf("%d", id->buf[i]);
  }
  printf("\n\n");

  return ROK;
}

/*******************************************************************
 *
 * @brief Fills the initiating IE for E2 Setup Request
 *
 * @details
 *
 *    Function : fillE2SetupReq
 *
 * Functionality:Fills the Initiating message for
 *               E2SetupRequest
 *
 * @params[in] E2setupRequest_t *e2SetupReq,
 *             uint8_t *idx
 * @return ROK     - success
 *         RFAILED - failure
 *
 ******************************************************************/

uint8_t fillE2SetupReq(E2setupRequest_t **e2SetupReq, uint8_t *idx)
{
   uint8_t elementCnt = 0;
   uint8_t idx2 = 0;
   uint8_t ret = ROK;

   if(*e2SetupReq != NULLP)
   {
      elementCnt = 1;
      (*e2SetupReq)->protocolIEs.list.count = elementCnt;
      (*e2SetupReq)->protocolIEs.list.size = \
              elementCnt * sizeof(E2setupRequestIEs_t);

      /* Initialize the E2Setup members */
      DU_ALLOC((*e2SetupReq)->protocolIEs.list.array, \
               (*e2SetupReq)->protocolIEs.list.size);
      if((*e2SetupReq)->protocolIEs.list.array == NULLP)
      {
         DU_LOG("\nE2AP : Memory allocation failed for array elements");
         ret = RFAILED;
      }
      else
      {
         for(*idx = 0; *idx < elementCnt; (*idx)++)
         {
            DU_ALLOC((*e2SetupReq)->protocolIEs.list.array[*idx],\
               sizeof(E2setupRequestIEs_t));
            if((*e2SetupReq)->protocolIEs.list.array[*idx] == NULLP)
            {
               DU_LOG("\nE2AP : Memory allocation failed for arrayidx [%d]", *idx);
               ret = RFAILED;
            }
            else
            {
               /* GlobalE2node_gNB_ID */
               (*e2SetupReq)->protocolIEs.list.array[idx2]->id = \
                      ProtocolIE_IDE2_id_GlobalE2node_ID;
               (*e2SetupReq)->protocolIEs.list.array[idx2]->criticality = \
                            CriticalityE2_reject;
               (*e2SetupReq)->protocolIEs.list.array[idx2]->value.present =\
                             E2setupRequestIEs__value_PR_GlobalE2node_ID;
               (*e2SetupReq)->protocolIEs.list.array[idx2]->value.choice.\
                  GlobalE2node_ID.present = GlobalE2node_ID_PR_gNB;

               DU_ALLOC((*e2SetupReq)->protocolIEs.list.array[idx2]->value.choice.\
                  GlobalE2node_ID.choice.gNB, sizeof(GlobalE2node_gNB_ID_t));
               if((*e2SetupReq)->protocolIEs.list.array[idx2]->value.choice.\
                  GlobalE2node_ID.choice.gNB == NULLP)
               {
                  DU_LOG("\nE2AP : Memory allocation failed for gNbId");
                  ret = RFAILED;
               }
               else
               {
                  ret = BuildGlobalgNBId((*e2SetupReq)->protocolIEs.list.array[idx2]->value.\
                     choice.GlobalE2node_ID.choice.gNB);
                  if(ret != ROK)
                  {
                     ret = RFAILED;
                  }
               }

            }
         }
      }
    }
    else
    {
       ret = RFAILED;
       DU_LOG("\nE2AP : Passed e2SetupReq is NULL");
    }
    return ret;
}

/*******************************************************************
 *
 * @brief Builds and Send the E2SetupRequest
 *
 * @details
 *
 *    Function : BuildAndSendE2SetupReq
 *
 * Functionality:Fills the E2SetupRequest
 *
 * @return ROK     - success
 *         RFAILED - failure
 *
 ******************************************************************/

uint8_t BuildAndSendE2SetupReq()
{
   uint8_t idx = 0;
   uint8_t ret = ROK;
   E2AP_PDU_t        *e2apMsg = NULLP;
   E2setupRequest_t  *e2SetupReq = NULLP;
   asn_enc_rval_t     encRetVal;       /* Encoder return value */

   DU_LOG("\nE2AP : Building E2 Setup Request\n");
   do
   {
      DU_ALLOC(e2apMsg, sizeof(E2AP_PDU_t));
      if(e2apMsg == NULLP)
      {
         DU_LOG("\nE2AP : Memory allocation for E2AP-PDU failed");
         break;
      }
      e2apMsg->present = E2AP_PDU_PR_initiatingMessage;
      DU_ALLOC(e2apMsg->choice.initiatingMessage, sizeof(InitiatingMessageE2_t));
      if(e2apMsg->choice.initiatingMessage == NULLP)
      {
         DU_LOG("\nE2AP : Memory allocation for E2AP-PDU failed");
         DU_FREE(e2apMsg, sizeof(E2AP_PDU_t));
         return RFAILED;
      }
      e2apMsg->choice.initiatingMessage->criticality = CriticalityE2_reject;
      e2apMsg->choice.initiatingMessage->procedureCode = ProcedureCodeE2_id_E2setup;
      e2apMsg->choice.initiatingMessage->value.present = InitiatingMessageE2__value_PR_E2setupRequest;
      e2SetupReq = &e2apMsg->choice.initiatingMessage->value.choice.E2setupRequest;

      ret = fillE2SetupReq(&e2SetupReq, &idx);
      if(ret != ROK)
      {
         DU_LOG("\nE2AP : fillE2SetupReq() failed");
         break;
      }
      /* Prints the Msg formed */
      xer_fprint(stdout, &asn_DEF_E2AP_PDU, e2apMsg);

      memset((uint8_t *)encBuf, 0, ENC_BUF_MAX_LEN);
      encBufSize = 0;
      encRetVal = aper_encode(&asn_DEF_E2AP_PDU, 0, e2apMsg, PrepFinalEncBuf,\
                      encBuf);
      if(encRetVal.encoded == ENCODE_FAIL)
      {
         DU_LOG("\nE2AP : Could not encode E2SetupRequest structure (at %s)\n",\
            encRetVal.failed_type ? encRetVal.failed_type->name : "unknown");
         break;
      }
      else
      {
         DU_LOG("\nE2AP : Created APER encoded buffer for E2SetupRequest\n");
         for(int i=0; i< encBufSize; i++)
         {
            printf("%x",encBuf[i]);
         }
      }
      if(SendE2APMsg(DU_APP_MEM_REGION, DU_POOL) != ROK)
      {
         DU_LOG("\nE2AP : Sending E2 Setup request failed");
      }

    break;
   }while(true);

   deAllocateE2SetupReqMsg(e2apMsg, e2SetupReq, idx);
   return ret;
}/* End of BuildAndSendE2SetupReq */

/*******************************************************************
 *
 * @brief De Allocate E2 Setup Request Message
 *
 * @details
 *
 *    Function : deAllocateE2SetupReqMsg
 *
 *    Functionality: De-Allocating E2 Setup request Message
 *
 * @params[in] E2AP_PDU_t *e2apMsg
 *             E2setupRequest_t *e2SetupReq
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/

uint8_t deAllocateE2SetupReqMsg(E2AP_PDU_t *e2apMsg, \
   E2setupRequest_t *e2SetupReq, uint8_t idx)
{
   uint8_t idx2;
   uint8_t ret = ROK;
   /* De-allocating Memory */
   if(e2apMsg != NULLP)
   {
      if(e2apMsg->choice.initiatingMessage != NULLP)
      {
         if(e2SetupReq->protocolIEs.list.array != NULLP)
         {
            for(idx2 = 0; idx2 < idx; idx2++)
            {
               if(e2SetupReq->protocolIEs.list.array[idx2] != NULLP)
               {
                  switch(e2SetupReq->protocolIEs.list.array[idx2]->id)
                  {
                     case ProtocolIE_IDE2_id_GlobalE2node_ID:
                     {
                        if(e2SetupReq->protocolIEs.list.array[idx2]->\
                             value.choice.GlobalE2node_ID.choice.gNB != NULLP)
                        {
                              GlobalE2node_gNB_ID_t *gNbId = NULLP;
                              gNbId = e2SetupReq->protocolIEs.list.array[idx2]->\
                                         value.choice.GlobalE2node_ID.choice.gNB;
                              if(gNbId->global_gNB_ID.plmn_id.buf != NULLP)
                              {
                                 if(gNbId->global_gNB_ID.gnb_id.choice.gnb_ID.buf != NULLP)
                                 {
                                    DU_FREE(gNbId->global_gNB_ID.gnb_id.choice.gnb_ID.buf,\
                                       gNbId->global_gNB_ID.gnb_id.choice.gnb_ID.size);
                                 }
                                 DU_FREE(gNbId->global_gNB_ID.plmn_id.buf,\
                                            gNbId->global_gNB_ID.plmn_id.size);
                              }
                              DU_FREE(e2SetupReq->protocolIEs.list.array[idx2]->value.\
                                choice.GlobalE2node_ID.choice.gNB, sizeof(GlobalE2node_gNB_ID_t));
                        }
                        DU_FREE(e2SetupReq->protocolIEs.list.array[idx2],\
                            sizeof(E2setupRequestIEs_t));
                        break;
                     }
                     default:
                        DU_LOG("\n E2AP: Invalid event at e2SetupRequet %ld ",\
	     	           (e2SetupReq->protocolIEs.list.array[idx2]->id));
                        break;
                  }
               }
            }
            DU_FREE(e2SetupReq->protocolIEs.list.array, e2SetupReq->protocolIEs.list.size);
         }
         DU_FREE(e2apMsg->choice.initiatingMessage, sizeof(InitiatingMessageE2_t));
      }
      DU_FREE(e2apMsg, sizeof(E2AP_PDU_t));
   }
   return ret;
}
/*******************************************************************
 *
 * @brief Builds Ric Request Id
 *
 * @details
 *
 *    Function : BuildRicRequestId
 *
 *    Functionality: Building the Ric Request Id
 *
 * @params[in] RICrequestID_t *ricReqId
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/

uint8_t BuildRicRequestId(RICrequestID_t *ricReqId)
{
   if(ricReqId != NULLP)
   {
      ricReqId->ricRequestorID = 1;
      ricReqId->ricInstanceID  = 1;
   }
	else
	{
	   return RFAILED;
	}
   return ROK;
}

/*******************************************************************
 *
 * @brief Fills the mandatory RicAdmitted List Items
 *
 * @details
 *
 *    Function : fillRicAdmitList
 *
 *    Functionality: Fills the mandatory Ric Admitted List Items
 *
 * @params[in] RICaction_Admitted_ItemIEs_t *ricAdmitItems
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/

uint8_t fillRicAdmitList(RICaction_Admitted_ItemIEs_t *ricAdmitItems)
{

   if(ricAdmitItems != NULLP)
   {
      ricAdmitItems->id = ProtocolIE_IDE2_id_RICaction_Admitted_Item;
      ricAdmitItems->criticality = CriticalityE2_reject;
      ricAdmitItems->value.present = RICaction_Admitted_ItemIEs__value_PR_RICaction_Admitted_Item;
      ricAdmitItems->value.choice.RICaction_Admitted_Item.ricActionID = 1;
   }
	else
	{
	   return RFAILED;
	}
   return ROK;
}
/*******************************************************************
 *
 * @brief Builds the mandatory RicAdmitted List Params
 *
 * @details
 *
 *    Function : BuildRicAdmitList
 *
 *    Functionality: Builds the mandatory Ric Admitted List Params
 *
 * @params[in] RICaction_Admitted_List_t *admitListPtr
 * @return ROK     - success
 *         RFAILED - failure
 *
 * ****************************************************************/

uint8_t BuildRicAdmitList(RICaction_Admitted_List_t *admitListPtr)
{
   uint8_t idx ;
   uint8_t elementCnt;
	uint8_t ret= ROK;
   elementCnt = 1;

   if(admitListPtr == NULLP)
	{
	    DU_LOG("\nE2AP : Memory allocation for RIC Admit List failed");
	    ret = RFAILED;
	}
   else
	{
	   admitListPtr->list.count = elementCnt;
      admitListPtr->list.size  = elementCnt * sizeof(RICaction_Admitted_ItemIEs_t);
      DU_ALLOC(admitListPtr->list.array, admitListPtr->list.size);
      if(admitListPtr->list.array == NULLP)
      {
         DU_LOG("\nE2AP : Memory allocation for RIC Admit List failed");
         ret = RFAILED;
      }
		else
		{
	      for(idx=0 ; idx<elementCnt ; idx++ )
	      {
            DU_ALLOC(admitListPtr->list.array[idx], sizeof(RICaction_Admitted_ItemIEs_t));
		      if(admitListPtr->list.array[idx] == NULLP)
		      {
		         ret = RFAILED;
		      }
			}
			if(ret != RFAILED)
			{
	         idx=0;
            fillRicAdmitList((RICaction_Admitted_ItemIEs_t *)admitListPtr->list.array[idx]);
         }
		}
	}
   return ret;
}
/*******************************************************************
 *
 * @breif Deallocation of BuildAndSendRicSubscriptionRsp memory
 *
 * @details
 *
 *    Function : FreeRicSubscriptionRsp
 *
 * Functionality:Free the RicSubscriptionRsp
 *
 * @param[in] E2AP_PDU_t *e2apRicMsg
 *
 * @return void
 *
 *
 ******************************************************************/
void FreeRicSubscriptionRsp(E2AP_PDU_t  *e2apRicMsg)
{
	  RICsubscriptionResponse_t  *ricSubscriptionRsp= NULLP;
     uint8_t idx=0;
	  uint8_t idx1=0;
	  RICaction_Admitted_List_t *admitListPtr;

	  if(e2apRicMsg != NULLP)
	  {
	     if(e2apRicMsg->choice.successfulOutcome != NULLP)
		  {
		     ricSubscriptionRsp = &e2apRicMsg->choice.successfulOutcome->value.choice.RICsubscriptionResponse;
			  if(ricSubscriptionRsp)
			  {
			     if(ricSubscriptionRsp->protocolIEs.list.array != NULLP)
				  {
				     for(idx=0; idx<ricSubscriptionRsp->protocolIEs.list.count; idx++)
					  {
					     if(ricSubscriptionRsp->protocolIEs.list.array[idx] != NULLP)
						  {
						     switch(ricSubscriptionRsp->protocolIEs.list.array[idx]->id)
							  {
							     case ProtocolIE_IDE2_id_RICrequestID:
								       break;

								  case ProtocolIE_IDE2_id_RANfunctionID:
								       break;

								  case ProtocolIE_IDE2_id_RICactions_Admitted:
						        {
						            admitListPtr = &ricSubscriptionRsp->protocolIEs.list.\
									   array[idx]->value.choice.RICaction_Admitted_List;
									   if(admitListPtr->list.array != NULLP)
									   {
									       for(idx1=0 ; idx1<admitListPtr->list.count; idx1++ )
									       {
									          if(admitListPtr->list.array[idx1] != NULLP)
						                   {
						          				 DU_FREE(admitListPtr->list.array[idx1],
									             sizeof(RICaction_Admitted_ItemIEs_t));
											    }
									       }
										    DU_FREE(admitListPtr->list.array, admitListPtr->list.size);
                               }
										 break;
								    }
									 default:
									    break;
                        }
						      DU_FREE(ricSubscriptionRsp->protocolIEs.list.array[idx], \
							   sizeof(RICsubscriptionResponse_IEs_t));
						   }
					   }
					   DU_FREE(ricSubscriptionRsp->protocolIEs.list.array, \
					   ricSubscriptionRsp->protocolIEs.list.size);
               }
            }
			   DU_FREE(e2apRicMsg->choice.successfulOutcome, sizeof(SuccessfulOutcomeE2_t));
		   }
         DU_FREE(e2apRicMsg, sizeof(E2AP_PDU_t));
	  }
}
 /*******************************************************************
 *
 * @brief Builds and Send the RicSubscriptionRsp
 *
 * @details
 *
 *    Function : BuildAndSendRicSubscriptionRsp
 *
 * functionality:Fills the RicSubscriptionRsp
 *
 * @return ROK     - success
 *         RFAILED - failure
 *
 ******************************************************************/
uint8_t  FillRicSubscriptionRsp(RICsubscriptionResponse_t  *ricSubscriptionRsp )
{
   uint8_t idx=0;
	uint8_t ret = ROK;
	uint8_t elementCnt = 0;
	uint8_t BuildRicRequestIdret=ROK;
	uint8_t BuildRicAdmitListret=ROK;

   elementCnt=3;
	ricSubscriptionRsp->protocolIEs.list.count = elementCnt;
	ricSubscriptionRsp->protocolIEs.list.size  = elementCnt * sizeof(RICsubscriptionResponse_IEs_t);
   DU_ALLOC(ricSubscriptionRsp->protocolIEs.list.array, \
	ricSubscriptionRsp->protocolIEs.list.size);
	if(ricSubscriptionRsp->protocolIEs.list.array == NULLP)
	{
      DU_LOG("\nE2AP : Memory allocation for FillRicSubscriptionRsp  failed");
		ret = RFAILED;
	}
	else
	{
	   for(idx=0; idx<ricSubscriptionRsp->protocolIEs.list.count; idx++)
	   {
	      DU_ALLOC(ricSubscriptionRsp->protocolIEs.list.array[idx], \
	   	sizeof(RICsubscriptionResponse_IEs_t));
	   	if(ricSubscriptionRsp->protocolIEs.list.array[idx] == NULLP)
			{
		   	ret = RFAILED;
			}
		}
		if(ret != RFAILED)
		{

			idx=0;
   		ricSubscriptionRsp->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICrequestID;
   		ricSubscriptionRsp->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
   		ricSubscriptionRsp->protocolIEs.list.array[idx]->value.present =\
   		RICsubscriptionRequest_IEs__value_PR_RICrequestID;
   		BuildRicRequestIdret =
   		BuildRicRequestId(&ricSubscriptionRsp->protocolIEs.list.array[idx]->value.choice.RICrequestID);
   		if(BuildRicRequestIdret != ROK)
   		{
     			ret = RFAILED;
   		}
	      else
			{
				idx++;
				ricSubscriptionRsp->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RANfunctionID;
				ricSubscriptionRsp->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
				ricSubscriptionRsp->protocolIEs.list.array[idx]->value.present =\
				RICsubscriptionRequest_IEs__value_PR_RANfunctionID;
				ricSubscriptionRsp->protocolIEs.list.array[idx]->value.choice.RANfunctionID = 1;

				idx++;
				ricSubscriptionRsp->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICactions_Admitted;
				ricSubscriptionRsp->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
				ricSubscriptionRsp->protocolIEs.list.array[idx]->value.present =\
				RICsubscriptionResponse_IEs__value_PR_RICaction_Admitted_List;
				BuildRicAdmitListret =
				BuildRicAdmitList(&ricSubscriptionRsp->protocolIEs.list.array[idx]->value.choice.RICaction_Admitted_List);
				if(BuildRicAdmitListret != ROK)
				{
	   			ret = RFAILED;
				}
			}
		}
	}
   return ret;
}
/*******************************************************************
 *
 * @brief Builds and Send the RicSubscriptionRsp
 *
 * @details
 *
 *    Function : BuildAndSendRicSubscriptionRsp
 *
 * Functionality:Fills the RicSubscriptionRsp
 *
 * @return ROK     - success
 *         RFAILED - failure
 *
 ******************************************************************/

uint8_t BuildAndSendRicSubscriptionRsp()
{

   E2AP_PDU_t         *e2apRicMsg = NULLP;
   RICsubscriptionResponse_t  *ricSubscriptionRsp=NULLP;
   asn_enc_rval_t     encRetVal;
   uint8_t ret = RFAILED;
   uint8_t FillRicricSubscriptionRspret;

   while(true)
   {
        DU_LOG("\nE2AP : Building RIC Subscription Response\n");

        DU_ALLOC(e2apRicMsg, sizeof(E2AP_PDU_t));
        if(e2apRicMsg == NULLP)
        {
           DU_LOG("\nE2AP : Memory allocation for E2AP-PDU failed");
           break;
        }
        e2apRicMsg->present =  E2AP_PDU_PR_successfulOutcome;
        DU_ALLOC(e2apRicMsg->choice.successfulOutcome, sizeof(SuccessfulOutcomeE2_t));
        if(e2apRicMsg->choice.successfulOutcome == NULLP)
        {
             DU_LOG("\nE2AP : Memory allocation for Ric subscription Response failed");
             break;
        }

        e2apRicMsg->choice.successfulOutcome->procedureCode = ProcedureCodeE2_id_RICsubscription;
        e2apRicMsg->choice.successfulOutcome->criticality = CriticalityE2_reject;
        e2apRicMsg->choice.successfulOutcome->value.present = \
         SuccessfulOutcomeE2__value_PR_RICsubscriptionResponse;
        ricSubscriptionRsp = &e2apRicMsg->choice.successfulOutcome->value.choice.RICsubscriptionResponse;

		  FillRicricSubscriptionRspret = FillRicSubscriptionRsp(ricSubscriptionRsp);
        if(FillRicricSubscriptionRspret != ROK)
		  {
		     DU_LOG("\nE2AP : Memory allocation for RICsubscriptionResponseIE failed");
		     break;
		  }

       /* Prints the Msg formed */
        xer_fprint(stdout, &asn_DEF_E2AP_PDU, e2apRicMsg);

        memset((uint8_t *)encBuf, 0, ENC_BUF_MAX_LEN);
        encBufSize = 0;
        encRetVal = aper_encode(&asn_DEF_E2AP_PDU, 0, e2apRicMsg, PrepFinalEncBuf,\
               encBuf);
        if(encRetVal.encoded == ENCODE_FAIL)
        {
	         DU_LOG("\nE2AP : Could not encode RIC Subscription Response structure (at %s)\n",\
			   encRetVal.failed_type ? encRetVal.failed_type->name : "unknown");
	         break;
        }
        else
        {
            DU_LOG("\nE2AP : Created APER encoded buffer for RIC subscription response \n");
	         for(int i=0; i< encBufSize; i++)
	         {
		          printf("%x",encBuf[i]);
	         }
        }

        if(SendE2APMsg(DU_APP_MEM_REGION, DU_POOL) != ROK)
        {
            DU_LOG("\nE2AP : Sending RIC Subscription Response failed");
            break;
        }

		  ret = ROK;
		  break;

    }
	 FreeRicSubscriptionRsp(e2apRicMsg);

	 return ret;
 }
/******************************************************************
*
* @brief Processes E2 Setup Response sent by RIC
*
* @details
*
*    Function : procE2SetupRsp
*
*    Functionality: Processes E2 Setup Response sent by RIC
*
* @params[in] E2AP_PDU_t ASN decoded E2AP message
* @return ROK     - success
*         RFAILED - failure
*
* ****************************************************************/
uint8_t procE2SetupRsp(E2AP_PDU_t *e2apMsg)
{
   uint8_t idx;
   uint32_t recvBufLen;
   E2setupResponse_t *e2SetRspMsg;

   DU_LOG("\nE2AP : E2 Setup Response received");
   duCb.e2Status = TRUE; //Set E2 status as true
   e2SetRspMsg = &e2apMsg->choice.successfulOutcome->value.choice.E2setupResponse;

   for(idx=0; idx<e2SetRspMsg->protocolIEs.list.count; idx++)
   {
      switch(e2SetRspMsg->protocolIEs.list.array[idx]->id)
      {
         case ProtocolIE_IDE2_id_GlobalRIC_ID:
         {
            /* To store the Ric Id Params */
            recvBufLen = sizeof(e2SetRspMsg->protocolIEs.list.array[idx]->value\
				 .choice.GlobalRIC_ID.pLMN_Identity.size);
				e2apMsgDb.plmn = NULLP;
            DU_ALLOC(e2apMsgDb.plmn, recvBufLen);
				if(e2apMsgDb.plmn)
				{
				   memcpy(e2apMsgDb.plmn, e2SetRspMsg->protocolIEs.list.array[idx]\
					 ->value.choice.GlobalRIC_ID.pLMN_Identity.buf, recvBufLen);
				}
            bitStringToInt(&e2SetRspMsg->protocolIEs.list.array[idx]->value.choice.GlobalRIC_ID.ric_ID, &e2apMsgDb.ricId);
            /*TODO : e2apMsgDb.plmn memory to be deallocated after the usage */
			   break;
         }
         default:
            DU_LOG("\nE2AP : Invalid IE received in E2SetupRsp:%ld",
                  e2SetRspMsg->protocolIEs.list.array[idx]->id);
            break;
      }
   }
   return ROK;
}

/******************************************************************
*
* @brief Processes RIC Subscription Req sent by RIC
*
* @details
*
*    Function : procRicSubsReq
*
*    Functionality: Processes E2 Setup Response sent by CU
*
* @params[in] E2AP_PDU_t ASN decoded E2AP message
* @return ROK     - success
*         RFAILED - failure
*
* ****************************************************************/

uint8_t procRicSubsReq(E2AP_PDU_t *e2apMsg)
{
   uint8_t idx;
   uint8_t ied;
   uint8_t ret = ROK;
   uint32_t recvBufLen;
   RICsubscriptionRequest_t *ricSubsReq;
   RICaction_ToBeSetup_ItemIEs_t *actionItem;

   DU_LOG("\nE2AP : Ric Subscription request received");
   ricSubsReq = &e2apMsg->choice.initiatingMessage->value.choice.RICsubscriptionRequest;

   for(idx=0; idx<ricSubsReq->protocolIEs.list.count; idx++)
   {
      switch(ricSubsReq->protocolIEs.list.array[idx]->id)
      {
         case ProtocolIE_IDE2_id_RICrequestID:
         {
            e2apMsgDb.ricReqId = ricSubsReq->protocolIEs.list.array[idx]->\
                                   value.choice.RICrequestID.ricRequestorID;
            e2apMsgDb.ricInstanceId = ricSubsReq->protocolIEs.list.array[idx]-> \
                                       value.choice.RICrequestID.ricInstanceID;
            break;
         }
         case ProtocolIE_IDE2_id_RANfunctionID:
         {
            e2apMsgDb.ranFuncId = ricSubsReq->protocolIEs.list.array[idx]-> \
                                   value.choice.RANfunctionID;
            break;
         }
         case ProtocolIE_IDE2_id_RICsubscriptionDetails:
         {
  			    recvBufLen = ricSubsReq->protocolIEs.list.array[idx]->value\
  				 .choice.RICsubscriptionDetails.ricEventTriggerDefinition.size;
  			    e2apMsgDb.ricEventTrigger = NULLP;
  				  DU_ALLOC(e2apMsgDb.ricEventTrigger, recvBufLen);

            /*TODO : e2apMsgDb.ricEventTrigger memory to be deallocated after the usage */
  				  if(e2apMsgDb.ricEventTrigger)
  			    {
  				    memcpy(e2apMsgDb.ricEventTrigger, ricSubsReq->protocolIEs.list.array[idx]\
  					    ->value.choice.RICsubscriptionDetails.ricEventTriggerDefinition.buf, \
  					    recvBufLen);
  	      //printf("\nricEventTrigger, recvBufLen %d, size %d\n", recvBufLen, ricSubsReq->protocolIEs.list.array[idx]->value.choice.RICsubscriptionDetails.ricEventTriggerDefinition.size);
              printf("\nlen %d, ricEventTrigger %s\n", strlen(e2apMsgDb.ricEventTrigger), e2apMsgDb.ricEventTrigger);

              float trigger_timer = ((float) atoi(e2apMsgDb.ricEventTrigger) / 1000.0);
              printf("trigger_timer %f seconds\n", trigger_timer);

              // print ricRequestId, this should be the xApp ID
              printf("e2apMsgDb.ricReqId %" PRIu32 "\n", e2apMsgDb.ricReqId);

              float *report_timer = (float*) calloc(1, sizeof(float));
              report_timer[0] = trigger_timer;

              uint32_t *ric_req_id = (uint32_t*) calloc(1, sizeof(uint32_t));
              ric_req_id[0] = e2apMsgDb.ricReqId;

              handleTimer(report_timer, ric_req_id);
  			    }

            actionItem =(RICaction_ToBeSetup_ItemIEs_t *)ricSubsReq->protocolIEs.list\
  				  .array[idx]->value.choice.RICsubscriptionDetails.ricAction_ToBeSetup_List\
  				  .list.array[0];

            for(ied = 0; ied < ricSubsReq->protocolIEs.list.array[idx]->value.choice.\
            RICsubscriptionDetails.ricAction_ToBeSetup_List.list.count; ied++)
            {
              switch(actionItem->id)
              {
                case ProtocolIE_IDE2_id_RICaction_ToBeSetup_Item:
                {
                  e2apMsgDb.ricActionId = actionItem->value.choice.RICaction_ToBeSetup_Item.ricActionID;
                  e2apMsgDb.ricActionType = actionItem->value.choice.RICaction_ToBeSetup_Item.ricActionType;
                  break;
                }
                default:
                  DU_LOG("\nE2AP : Invalid IE received in RicSetupLst:%ld",actionItem->id);
                  break;
                }
            }

            break;
        }

        default:
            DU_LOG("\nE2AP : Invalid IE received in Ric SubsReq:%ld",
                  ricSubsReq->protocolIEs.list.array[idx]->id);
            break;
      }
   }
   ret = BuildAndSendRicSubscriptionRsp();

   return ret;
}

/**
 * Handle RIC control
 * TODO doxy
 */
uint8_t procRicControlReq(E2AP_PDU_t *e2apMsg)
{
   uint8_t idx;
   uint8_t ied;
   uint8_t ret = ROK;
   uint32_t recvBufLen;
   RICcontrolRequest_t *ricControlRequest;
   RICaction_ToBeSetup_ItemIEs_t *actionItem;

   DU_LOG("\nE2AP : RIC Control request received");
   ricControlRequest = &e2apMsg->choice.initiatingMessage->value.choice.RICcontrolRequest;

   DU_LOG("protocolIEs elements %d\n", ricControlRequest->protocolIEs.list.count);

   for (idx = 0; idx < ricControlRequest->protocolIEs.list.count; idx++)
   {
      switch(ricControlRequest->protocolIEs.list.array[idx]->id)
      {
         case ProtocolIE_IDE2_id_RICrequestID:
         {
            //DU_LOG("\nProtocolIE_IDE2_id_RICrequestID");
            e2apMsgDb.ricReqId = ricControlRequest->protocolIEs.list.array[idx]->\
                                   value.choice.RICrequestID.ricRequestorID;
            e2apMsgDb.ricInstanceId = ricControlRequest->protocolIEs.list.array[idx]-> \
                                       value.choice.RICrequestID.ricInstanceID;

	    DU_LOG("\nricReqID %ld", e2apMsgDb.ricReqId);
	    DU_LOG("\nricInstanceId %ld", e2apMsgDb.ricInstanceId);
            break;
         }
         case ProtocolIE_IDE2_id_RANfunctionID:
         {
            //DU_LOG("\nProtocolIE_IDE2_id_RANfunctionID");
            e2apMsgDb.ranFuncId = ricControlRequest->protocolIEs.list.array[idx]-> \
                                   value.choice.RANfunctionID;

	    DU_LOG("\nranFuncId %ld", e2apMsgDb.ranFuncId);
            break;
         }
         case ProtocolIE_IDE2_id_RICcontrolMessage:
         {
            DU_LOG("\nProtocolIE_IDE2_id_RICcontrolMessage");
            recvBufLen = ricControlRequest->protocolIEs.list.array[idx]->value\
               .choice.RICcontrolMessage.size;
            e2apMsgDb.ricEventTrigger = NULLP;
            DU_ALLOC(e2apMsgDb.ricEventTrigger, recvBufLen);

            if(e2apMsgDb.ricEventTrigger)
            {
               memcpy(e2apMsgDb.ricEventTrigger, ricControlRequest->protocolIEs.list.array[idx]\
                ->value.choice.RICcontrolMessage.buf, \
                recvBufLen);
            }

            DU_LOG("\nPrint content of ricEventTrigger (which should be the RICcontrolMessage)\n");
            DU_LOG(e2apMsgDb.ricEventTrigger);
            DU_LOG("\ne2apMsgDb.ricEventTrigger printed");

            // log message on file
            log_message((char*) e2apMsgDb.ricEventTrigger, "control", (int)recvBufLen);

            // write policies on config file
            if (strcmp((char*) e2apMsgDb.ricEventTrigger, "terminate") == 0) {
              stop_data_reporting_nrt_ric();
            }
            else {
              write_control_policies_c((char*) e2apMsgDb.ricEventTrigger);
            }

            break;
         }
         case ProtocolIE_IDE2_id_RICcontrolHeader:
         {
            DU_LOG("\nProtocolIE_IDE2_id_RICcontrolHeader");

            // TODO
            break;
         }
      }
   }
   return ret; // TODO update ret value in case of errors
}


 /*******************************************************************
 *
 * @brief Free the RicIndication Message
 *
 * @details
 *
 *    Function : FreeRicIndication
 *
 * Functionality: Free the RicIndication Message
 *
 * @return void
 *
 *
 ******************************************************************/
 void FreeRicIndication(E2AP_PDU_t  *e2apMsg)
 {
     uint8_t idx=0;
     RICindication_t *ricIndicationMsg= NULLP;


     if(e2apMsg != NULLP)
	  {
	      if(e2apMsg->choice.initiatingMessage != NULLP)
			{
				 ricIndicationMsg = &e2apMsg->choice.initiatingMessage->value.choice.RICindication;
			    if(ricIndicationMsg!= NULLP)
				 {
				     if(ricIndicationMsg->protocolIEs.list.array != NULLP)
					  {
					      for(idx=0; idx<ricIndicationMsg->protocolIEs.list.count; idx++)
					      {
						       if(ricIndicationMsg->protocolIEs.list.array[idx] != NULLP)
						       {
						           switch(ricIndicationMsg->protocolIEs.list.array[idx]->id)
									  {
									      case ProtocolIE_IDE2_id_RICrequestID:
											     break;

											case ProtocolIE_IDE2_id_RANfunctionID:
											     break;

											case ProtocolIE_IDE2_id_RICactionID:
											     break;

											case ProtocolIE_IDE2_id_RICindicationType:
											     break;

											case ProtocolIE_IDE2_id_RICindicationHeader:
											{
											    DU_FREE(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.buf,\
												 ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.size);
											    break;
											}
											case ProtocolIE_IDE2_id_RICindicationMessage:
											{
											    DU_FREE(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.buf,\
												 ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.size);
											    break;
											}
											default:
											   break;
									  }
									  DU_FREE(ricIndicationMsg->protocolIEs.list.array[idx],sizeof(RICindication_IEs_t));
							    }
					      }
					      DU_FREE(ricIndicationMsg->protocolIEs.list.array,ricIndicationMsg->protocolIEs.list.size);
						}
			  	 }
				 DU_FREE(e2apMsg->choice.initiatingMessage, sizeof(InitiatingMessageE2_t));
			}
	      DU_FREE(e2apMsg, sizeof(E2AP_PDU_t));
	  }
 }
 /*******************************************************************
  *
  * brief Fill the RicIndication Message
  *
  * @details
  *
  *    Function : FillRicIndication
  *
  * Functionality:Fills the RicIndication Message
  *
  * @return ROK     - success
  *         RFAILED - failure
  *
  ******************************************************************/
uint8_t FillRicIndication(RICindication_t *ricIndicationMsg)
{
   uint8_t elementCnt=0;
   uint8_t idx=0;
   uint8_t ret = ROK;
   elementCnt = 6;

	ricIndicationMsg->protocolIEs.list.count = elementCnt;
	ricIndicationMsg->protocolIEs.list.size  = elementCnt * sizeof(RICindication_t);
	/* Initialize the Ric Indication members */
	DU_ALLOC(ricIndicationMsg->protocolIEs.list.array, \
	ricIndicationMsg->protocolIEs.list.size);
	if(ricIndicationMsg->protocolIEs.list.array == NULLP)
	{
	    DU_LOG("\nE2AP : Memory allocation for RICindicationIEs failed");
	    ret = RFAILED;
	}
   else
	{
	   for(idx=0; idx<elementCnt; idx++)
      {
	      DU_ALLOC(ricIndicationMsg->protocolIEs.list.array[idx],\
	      sizeof(RICindication_IEs_t));
	      if(ricIndicationMsg->protocolIEs.list.array[idx] == NULLP)
	      {
	         DU_LOG("\nE2AP : Memory allocation for RICindicationIEs failed");
	         ret = RFAILED;
			}
	   }
		if(ret != RFAILED)
		{
         idx = 0;

	      ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICrequestID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
			                                RICindication_IEs__value_PR_RICrequestID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICrequestID.ricRequestorID =\
			e2apMsgDb.ricReqId;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICrequestID.ricInstanceID =\
			e2apMsgDb.ricInstanceId;

	      idx++;
	      ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RANfunctionID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
	      RICindication_IEs__value_PR_RANfunctionID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RANfunctionID =
			e2apMsgDb.ranFuncId;

	      idx++;
	      ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICactionID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
			RICindication_IEs__value_PR_RICactionID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICactionID =
			e2apMsgDb.ricActionId;

	      idx++;
	      ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICindicationType;
	      ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
			RICindication_IEs__value_PR_RICindicationType;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationType =
			e2apMsgDb.ricActionType;

			idx++;
         ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICindicationHeader;
         ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
	      RICindication_IEs__value_PR_RICindicationHeader;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.size = 3 *
	      sizeof(uint8_t);
         DU_ALLOC(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.buf ,\
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.size);
	      if(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.buf == NULLP)
	      {
	         DU_LOG("\nE2AP : Memory allocation for RICindicationIEs failed");
	         ret = RFAILED;
	      }
		   else
		   {
	         buildPlmnId(duCfgParam.srvdCellLst[0].duCellInfo.cellInfo.nrCgi.plmn, \
	         &ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader);
	         idx++;
            /* TO BE CHANGED: RIC INDICATION DATA */
            /* For now filling a dummy octect data, need to tested with PRBs*/
            ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICindicationMessage;
            ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
            ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
            RICindication_IEs__value_PR_RICindicationMessage;
            ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.size = 3 *
            sizeof(uint8_t);
            DU_ALLOC(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.buf ,\
   			ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.size);
				if(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.buf == NULLP)
				{
	    			DU_LOG("\nE2AP : Memory allocation for RICindicationIEs failed");
       			ret = RFAILED;
   			}
				else
				{
					buildPlmnId(duCfgParam.srvdCellLst[0].duCellInfo.cellInfo.nrCgi.plmn, \
					&ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage);
				}
			}
		}
	}
	return ret;
}

uint8_t FillRicIndicationReport(RICindication_t *ricIndicationMsg, char* payload, int payload_len, uint32_t ric_req_id)
{
   uint8_t elementCnt=0;
   uint8_t idx=0;
   uint8_t ret = ROK;
   elementCnt = 6;

	ricIndicationMsg->protocolIEs.list.count = elementCnt;
	ricIndicationMsg->protocolIEs.list.size  = elementCnt * sizeof(RICindication_t);
	/* Initialize the Ric Indication members */
	DU_ALLOC(ricIndicationMsg->protocolIEs.list.array, \
	ricIndicationMsg->protocolIEs.list.size);
	if(ricIndicationMsg->protocolIEs.list.array == NULLP)
	{
	    DU_LOG("\nE2AP : Memory allocation for RICindicationIEs failed");
	    ret = RFAILED;
	}
   else
	{
	   for(idx=0; idx<elementCnt; idx++)
      {
	      DU_ALLOC(ricIndicationMsg->protocolIEs.list.array[idx],\
	      sizeof(RICindication_IEs_t));
	      if(ricIndicationMsg->protocolIEs.list.array[idx] == NULLP)
	      {
	         DU_LOG("\nE2AP : Memory allocation for RICindicationIEs failed");
	         ret = RFAILED;
			}
	   }
		if(ret != RFAILED)
		{
         idx = 0;

	      ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICrequestID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
			                                RICindication_IEs__value_PR_RICrequestID;
	      
        // set ricReqId here otherwise it will get the one of the last received RIC indication
        ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICrequestID.ricRequestorID = ric_req_id;
        // ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICrequestID.ricRequestorID = e2apMsgDb.ricReqId;
	      
        ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICrequestID.ricInstanceID =\
			e2apMsgDb.ricInstanceId;

	      idx++;
	      ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RANfunctionID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
	      RICindication_IEs__value_PR_RANfunctionID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RANfunctionID =
			e2apMsgDb.ranFuncId;

	      idx++;
	      ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICactionID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
			RICindication_IEs__value_PR_RICactionID;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICactionID =
			e2apMsgDb.ricActionId;

	      idx++;
	      ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICindicationType;
	      ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
			RICindication_IEs__value_PR_RICindicationType;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationType = RICactionType_report;

			idx++;
         ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICindicationHeader;
         ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
	      RICindication_IEs__value_PR_RICindicationHeader;
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.size = 3 *
	      sizeof(uint8_t);
         DU_ALLOC(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.buf ,\
	      ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.size);
	      if(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader.buf == NULLP)
	      {
	         DU_LOG("\nE2AP : Memory allocation for RICindicationIEs failed");
	         ret = RFAILED;
	      }
		   else
		   {
	         buildPlmnId(duCfgParam.srvdCellLst[0].duCellInfo.cellInfo.nrCgi.plmn, \
	         &ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationHeader);
	         idx++;
            /* TO BE CHANGED: RIC INDICATION DATA */
            /* For now filling a dummy octect data, need to tested with PRBs*/
            ricIndicationMsg->protocolIEs.list.array[idx]->id = ProtocolIE_IDE2_id_RICindicationMessage;
            ricIndicationMsg->protocolIEs.list.array[idx]->criticality = CriticalityE2_reject;
            ricIndicationMsg->protocolIEs.list.array[idx]->value.present = \
            RICindication_IEs__value_PR_RICindicationMessage;
            // ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.size = 3 *
            // sizeof(uint8_t);
            ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.size = payload_len *
            sizeof(uint8_t);
            DU_ALLOC(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.buf ,\
   			ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.size);
				if(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.buf == NULLP)
				{
	    			DU_LOG("\nE2AP : Memory allocation for RICindicationIEs failed");
       			ret = RFAILED;
   			}
				else
				{
					//buildPlmnId(duCfgParam.srvdCellLst[0].duCellInfo.cellInfo.nrCgi.plmn, \
					&ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage);

          memcpy(ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.buf, payload,\
            ricIndicationMsg->protocolIEs.list.array[idx]->value.choice.RICindicationMessage.size);
				}
			}
		}
	}
	return ret;
}

/*******************************************************************
 *
 * @brief Builds and Send the RicIndication Message
 *
 * @details
 *
 *    Function : BuildAndSendRicIndication
 *
 * Functionality:Fills the RicIndication Message
 *
 * @return ROK     - success
 *         RFAILED - failure
 *
 ******************************************************************/

uint8_t BuildAndSendRicIndication()
{
   E2AP_PDU_t                 *e2apMsg = NULLP;
   RICindication_t            *ricIndicationMsg=NULLP;
   asn_enc_rval_t             encRetVal;        /* Encoder return value */
   uint8_t ret = RFAILED;
	uint8_t FillRicIndicationret = ROK;

	while(true)
	{
        DU_LOG("\nE2AP : Building Ric Indication Message\n");

        DU_ALLOC(e2apMsg, sizeof(E2AP_PDU_t));
        if(e2apMsg == NULLP)
        {
             DU_LOG("\nE2AP : Memory allocation for E2AP-PDU failed");
             break;
        }

        e2apMsg->present = E2AP_PDU_PR_initiatingMessage;
        DU_ALLOC(e2apMsg->choice.initiatingMessage, sizeof(InitiatingMessageE2_t));
        if(e2apMsg->choice.initiatingMessage == NULLP)
        {
             DU_LOG("\nE2AP : Memory allocation for E2AP-PDU failed");
             break;
        }
        e2apMsg->choice.initiatingMessage->procedureCode = ProcedureCodeE2_id_RICindication;
        e2apMsg->choice.initiatingMessage->criticality = CriticalityE2_reject;
        e2apMsg->choice.initiatingMessage->value.present = InitiatingMessageE2__value_PR_RICindication;

		  ricIndicationMsg = &e2apMsg->choice.initiatingMessage->value.choice.RICindication;

		  FillRicIndicationret = FillRicIndication(ricIndicationMsg);
        if(FillRicIndicationret != ROK)
		  {
		      break;
		  }
		  /* Prints the Msg formed */
        xer_fprint(stdout, &asn_DEF_E2AP_PDU, e2apMsg);
        memset((uint8_t *)encBuf, 0, ENC_BUF_MAX_LEN);
		  encBufSize = 0;
        encRetVal = aper_encode(&asn_DEF_E2AP_PDU, 0, e2apMsg, PrepFinalEncBuf,\
               encBuf);
        if(encRetVal.encoded == ENCODE_FAIL)
        {
	         DU_LOG("\nE2AP : Could not encode RIC Indication Message (at %s)\n",\
			   encRetVal.failed_type ? encRetVal.failed_type->name : "unknown");
	         break;
        }
        else
        {
           DU_LOG("\nE2AP : Created APER encoded buffer for RIC Indication Message \n");
	        for(int i=0; i< encBufSize; i++)
	        {
		         printf("%x",encBuf[i]);
	        }
        }

        if(SendE2APMsg(DU_APP_MEM_REGION, DU_POOL) != ROK)
        {
            DU_LOG("\nE2AP : Sending RIC Indication Message");

        }
		  ret = ROK;
		  break;
   }
   FreeRicIndication(e2apMsg);
	return ret;
}

uint8_t BuildAndSendRicIndicationReport(char* payload, int payload_len, uint32_t ric_req_id)
{
   E2AP_PDU_t                 *e2apMsg = NULLP;
   RICindication_t            *ricIndicationMsg=NULLP;
   asn_enc_rval_t             encRetVal;        /* Encoder return value */
   uint8_t ret = RFAILED;
	uint8_t FillRicIndicationret = ROK;

	while(true)
	{
        DU_LOG("\nE2AP : Building Ric Indication Message\n");

        DU_ALLOC(e2apMsg, sizeof(E2AP_PDU_t));
        if(e2apMsg == NULLP)
        {
             DU_LOG("\nE2AP : Memory allocation for E2AP-PDU failed");
             break;
        }

        e2apMsg->present = E2AP_PDU_PR_initiatingMessage;
        DU_ALLOC(e2apMsg->choice.initiatingMessage, sizeof(InitiatingMessageE2_t));
        if(e2apMsg->choice.initiatingMessage == NULLP)
        {
             DU_LOG("\nE2AP : Memory allocation for E2AP-PDU failed");
             break;
        }
        e2apMsg->choice.initiatingMessage->procedureCode = ProcedureCodeE2_id_RICindication;
        e2apMsg->choice.initiatingMessage->criticality = CriticalityE2_reject;
        e2apMsg->choice.initiatingMessage->value.present = InitiatingMessageE2__value_PR_RICindication;

		  ricIndicationMsg = &e2apMsg->choice.initiatingMessage->value.choice.RICindication;

		  FillRicIndicationret = FillRicIndicationReport(ricIndicationMsg, payload, payload_len, ric_req_id);
        if(FillRicIndicationret != ROK)
		  {
		      break;
		  }
		  /* Prints the Msg formed */
        xer_fprint(stdout, &asn_DEF_E2AP_PDU, e2apMsg);
        memset((uint8_t *)encBuf, 0, ENC_BUF_MAX_LEN);
		  encBufSize = 0;
        encRetVal = aper_encode(&asn_DEF_E2AP_PDU, 0, e2apMsg, PrepFinalEncBuf,\
               encBuf);
        if(encRetVal.encoded == ENCODE_FAIL)
        {
	         DU_LOG("\nE2AP : Could not encode RIC Indication Message (at %s)\n",\
			   encRetVal.failed_type ? encRetVal.failed_type->name : "unknown");
	         break;
        }
        else
        {
           DU_LOG("\nE2AP : Created APER encoded buffer for RIC Indication Message \n");
	        for(int i=0; i< encBufSize; i++)
	        {
		         printf("%x",encBuf[i]);
	        }
        }

        if(SendE2APMsg(DU_APP_MEM_REGION, DU_POOL) == ROK)
        {
          log_message((char*) payload, "report", payload_len);
          DU_LOG("\nE2AP : Sending RIC Indication Message");
        }
		  ret = ROK;
		  break;
   }
   FreeRicIndication(e2apMsg);
	return ret;
}

/*******************************************************************
*
* @brief Sends E2 msg over SCTP
*
* @details
*
*    Function : SendE2APMsg
*
*    Functionality: Sends E2 msg over SCTP
*
* @params[in] Region region
*             Pool pool
* @return ROK     - success
*         RFAILED - failure
*
* ****************************************************************/

uint8_t SendE2APMsg(Region region, Pool pool)
{
   Buffer *mBuf=NULLP;

   if(ODU_GET_MSG_BUF(region, pool, &mBuf) == ROK)
   {
      if(ODU_ADD_POST_MSG_MULT((Data *)encBuf, encBufSize, mBuf) == ROK)
      {
         ODU_PRINT_MSG(mBuf, 0,0);

         if(sctpSend(mBuf, E2_INTERFACE) != ROK)
         {
            DU_LOG("\nE2AP : SCTP Send for E2  failed");
            ODU_PUT_MSG_BUF(mBuf);
            return RFAILED;
         }
      }
      else
      {
         DU_LOG("\nE2AP : ODU_ADD_POST_MSG_MULT failed");
         ODU_PUT_MSG_BUF(mBuf);
         return RFAILED;
      }
      ODU_PUT_MSG_BUF(mBuf);
   }
   else
   {
      DU_LOG("\nE2AP : Failed to allocate memory");
      return RFAILED;
   }

   return ROK;
} /* SendE2APMsg */

/*******************************************************************
*
* @brief Handles received E2AP message and sends back response
*
* @details
*
*    Function : E2APMsgHdlr
*
*    Functionality:
*         - Decodes received E2AP control message
*         - Prepares response message, encodes and sends to SCTP
*
* @params[in]
* @return ROK     - success
*         RFAILED - failure
*
* ****************************************************************/
void E2APMsgHdlr(Buffer *mBuf)
{
   int i;
   char *recvBuf;
   MsgLen copyCnt;
   MsgLen recvBufLen;
   E2AP_PDU_t *e2apMsg;
   asn_dec_rval_t rval; /* Decoder return value */
   E2AP_PDU_t e2apasnmsg ;

   DU_LOG("\nE2AP : Received E2AP message buffer");
   ODU_PRINT_MSG(mBuf, 0,0);

   /* Copy mBuf into char array to decode it */
   ODU_FIND_MSG_LEN(mBuf, &recvBufLen);
   DU_ALLOC(recvBuf, (Size)recvBufLen);

   if(recvBuf == NULLP)
   {
      DU_LOG("\nE2AP : Memory allocation failed");
      return;
   }
   if(ODU_COPY_MSG_TO_FIX_BUF(mBuf, 0, recvBufLen, (Data *)recvBuf, &copyCnt) != ROK)
   {
      DU_LOG("\nE2AP : Failed while copying %d", copyCnt);
      return;
   }

   printf("\nE2AP : Received flat buffer to be decoded : ");
   for(i=0; i< recvBufLen; i++)
   {
        printf("%x",recvBuf[i]);
   }

   /* Decoding flat buffer into E2AP messsage */
   e2apMsg = &e2apasnmsg;
   memset(e2apMsg, 0, sizeof(E2AP_PDU_t));

   rval = aper_decode(0, &asn_DEF_E2AP_PDU, (void **)&e2apMsg, recvBuf, recvBufLen, 0, 0);
   DU_FREE(recvBuf, (Size)recvBufLen);

   if(rval.code == RC_FAIL || rval.code == RC_WMORE)
   {
      DU_LOG("\nE2AP : ASN decode failed");
      return;
   }
   printf("\n");
   xer_fprint(stdout, &asn_DEF_E2AP_PDU, e2apMsg);

   switch(e2apMsg->present)
   {
      case E2AP_PDU_PR_successfulOutcome:
      {
         switch(e2apMsg->choice.successfulOutcome->value.present)
         {
            case SuccessfulOutcomeE2__value_PR_E2setupResponse:
            {
               if(!duCb.e2Status)
               {
                 DU_LOG("\nE2AP : Store E2 setup response Params");
                 procE2SetupRsp(e2apMsg);
               }
	       break;
            }
            default:
            {
               DU_LOG("\nE2AP : Invalid type of intiating message [%d]", e2apMsg->choice.initiatingMessage->value.present);
               return;
            }
         }/* End of switch(successfulOutcome) */
         break;
      }
      case E2AP_PDU_PR_initiatingMessage:
      {
         switch(e2apMsg->choice.initiatingMessage->value.present)
         {
            case InitiatingMessageE2__value_PR_RICsubscriptionRequest:
            {
               DU_LOG("\nE2AP : Calling RIC Subscription Response");
	       if(procRicSubsReq(e2apMsg) == ROK)
               {
                  //BuildAndSendRicIndication();
                  char* payload = "Indication ACK\n";
                  int payload_len = strlen(payload);
                  BuildAndSendRicIndicationReport(payload, payload_len, e2apMsgDb.ricReqId);
               }
               break;
            }
            case InitiatingMessageE2__value_PR_RICcontrolRequest:
            {
               DU_LOG("\nE2AP : Calling RIC Control Request");
               if(procRicControlReq(e2apMsg) == ROK)
               {
                  // TODO
               }
               break;
            }
            default:
            {
               DU_LOG("\nE2AP : Invalid type of successfulOutcome message [%d]", e2apMsg->choice.successfulOutcome->value.present);
               return;
            }
         }/* End of switch(initiatingMessage) */
         break;
      }
      default:
      {
         DU_LOG("\nE2AP : Invalid type of e2apMsg->present [%d]",e2apMsg->present);
         return;
      }

   }/* End of switch(e2apMsg->present) */

} /* End of E2APMsgHdlr */

/**********************************************************************
         End of file
**********************************************************************/
