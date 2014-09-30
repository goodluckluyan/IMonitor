/* soapServer.cpp
   Generated by gSOAP 2.8.18 from webservice.h

Copyright(C) 2000-2014, Robert van Engelen, Genivia Inc. All Rights Reserved.
The generated code is released under one of the following licenses:
GPL or Genivia's license for commercial use.
This program is released under the GPL with the additional exemption that
compiling, linking, and/or using OpenSSL is allowed.
*/

#if defined(__BORLANDC__)
#pragma option push -w-8060
#pragma option push -w-8004
#endif
#include "soapH.h"

SOAP_SOURCE_STAMP("@(#) soapServer.cpp ver 2.8.18 2014-09-15 01:50:20 GMT")


extern "C" SOAP_FMAC5 int SOAP_FMAC6 soap_serve(struct soap *soap)
{
#ifndef WITH_FASTCGI
	unsigned int k = soap->max_keep_alive;
#endif
	do
	{
#ifndef WITH_FASTCGI
		if (soap->max_keep_alive > 0 && !--k)
			soap->keep_alive = 0;
#endif
		if (soap_begin_serve(soap))
		{	if (soap->error >= SOAP_STOP)
				continue;
			return soap->error;
		}
		if (soap_serve_request(soap) || (soap->fserveloop && soap->fserveloop(soap)))
		{
#ifdef WITH_FASTCGI
			soap_send_fault(soap);
#else
			return soap_send_fault(soap);
#endif
		}

#ifdef WITH_FASTCGI
		soap_destroy(soap);
		soap_end(soap);
	} while (1);
#else
	} while (soap->keep_alive);
#endif
	return SOAP_OK;
}

#ifndef WITH_NOSERVEREQUEST
extern "C" SOAP_FMAC5 int SOAP_FMAC6 soap_serve_request(struct soap *soap)
{
	soap_peek_element(soap);
	if (!soap_match_tag(soap, soap->tag, "mons:GetMontorState"))
		return soap_serve_mons__GetMontorState(soap);
	if (!soap_match_tag(soap, soap->tag, "mons:GetTMSState"))
		return soap_serve_mons__GetTMSState(soap);
	if (!soap_match_tag(soap, soap->tag, "mons:GetSMSState"))
		return soap_serve_mons__GetSMSState(soap);
	if (!soap_match_tag(soap, soap->tag, "mons:GetRaidtate"))
		return soap_serve_mons__GetRaidtate(soap);
	if (!soap_match_tag(soap, soap->tag, "mons:GetEthState"))
		return soap_serve_mons__GetEthState(soap);
	if (!soap_match_tag(soap, soap->tag, "mons:GetSwitchState"))
		return soap_serve_mons__GetSwitchState(soap);
	if (!soap_match_tag(soap, soap->tag, "mons:GetIngestSpeedLimit"))
		return soap_serve_mons__GetIngestSpeedLimit(soap);
	if (!soap_match_tag(soap, soap->tag, "mons:GetWorkState_CS"))
		return soap_serve_mons__GetWorkState_USCORECS(soap);
	return soap->error = SOAP_NO_METHOD;
}
#endif

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetMontorState(struct soap *soap)
{	struct mons__GetMontorState soap_tmp_mons__GetMontorState;
	struct mons__MontorStateRes ret;
	soap_default_mons__MontorStateRes(soap, &ret);
	soap_default_mons__GetMontorState(soap, &soap_tmp_mons__GetMontorState);
	if (!soap_get_mons__GetMontorState(soap, &soap_tmp_mons__GetMontorState, "mons:GetMontorState", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = mons__GetMontorState(soap, ret);
	if (soap->error)
		return soap->error;
	soap->encodingStyle = NULL;
	soap_serializeheader(soap);
	soap_serialize_mons__MontorStateRes(soap, &ret);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_mons__MontorStateRes(soap, &ret, "mons:MontorStateRes", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_mons__MontorStateRes(soap, &ret, "mons:MontorStateRes", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetTMSState(struct soap *soap)
{	struct mons__GetTMSState soap_tmp_mons__GetTMSState;
	struct mons__TmsStateRes ret;
	soap_default_mons__TmsStateRes(soap, &ret);
	soap_default_mons__GetTMSState(soap, &soap_tmp_mons__GetTMSState);
	if (!soap_get_mons__GetTMSState(soap, &soap_tmp_mons__GetTMSState, "mons:GetTMSState", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = mons__GetTMSState(soap, ret);
	if (soap->error)
		return soap->error;
	soap->encodingStyle = NULL;
	soap_serializeheader(soap);
	soap_serialize_mons__TmsStateRes(soap, &ret);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_mons__TmsStateRes(soap, &ret, "mons:TmsStateRes", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_mons__TmsStateRes(soap, &ret, "mons:TmsStateRes", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetSMSState(struct soap *soap)
{	struct mons__GetSMSState soap_tmp_mons__GetSMSState;
	struct mons__SMSState ret;
	soap_default_mons__SMSState(soap, &ret);
	soap_default_mons__GetSMSState(soap, &soap_tmp_mons__GetSMSState);
	if (!soap_get_mons__GetSMSState(soap, &soap_tmp_mons__GetSMSState, "mons:GetSMSState", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = mons__GetSMSState(soap, ret);
	if (soap->error)
		return soap->error;
	soap->encodingStyle = NULL;
	soap_serializeheader(soap);
	soap_serialize_mons__SMSState(soap, &ret);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_mons__SMSState(soap, &ret, "mons:SMSState", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_mons__SMSState(soap, &ret, "mons:SMSState", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetRaidtate(struct soap *soap)
{	struct mons__GetRaidtate soap_tmp_mons__GetRaidtate;
	struct mons__RaidStateRes ret;
	soap_default_mons__RaidStateRes(soap, &ret);
	soap_default_mons__GetRaidtate(soap, &soap_tmp_mons__GetRaidtate);
	if (!soap_get_mons__GetRaidtate(soap, &soap_tmp_mons__GetRaidtate, "mons:GetRaidtate", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = mons__GetRaidtate(soap, ret);
	if (soap->error)
		return soap->error;
	soap->encodingStyle = NULL;
	soap_serializeheader(soap);
	soap_serialize_mons__RaidStateRes(soap, &ret);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_mons__RaidStateRes(soap, &ret, "mons:RaidStateRes", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_mons__RaidStateRes(soap, &ret, "mons:RaidStateRes", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetEthState(struct soap *soap)
{	struct mons__GetEthState soap_tmp_mons__GetEthState;
	struct mons__ethstate ret;
	soap_default_mons__ethstate(soap, &ret);
	soap_default_mons__GetEthState(soap, &soap_tmp_mons__GetEthState);
	if (!soap_get_mons__GetEthState(soap, &soap_tmp_mons__GetEthState, "mons:GetEthState", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = mons__GetEthState(soap, ret);
	if (soap->error)
		return soap->error;
	soap->encodingStyle = NULL;
	soap_serializeheader(soap);
	soap_serialize_mons__ethstate(soap, &ret);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_mons__ethstate(soap, &ret, "mons:ethstate", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_mons__ethstate(soap, &ret, "mons:ethstate", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetSwitchState(struct soap *soap)
{	struct mons__GetSwitchState soap_tmp_mons__GetSwitchState;
	struct mons__SwitchStateRes ret;
	soap_default_mons__SwitchStateRes(soap, &ret);
	soap_default_mons__GetSwitchState(soap, &soap_tmp_mons__GetSwitchState);
	if (!soap_get_mons__GetSwitchState(soap, &soap_tmp_mons__GetSwitchState, "mons:GetSwitchState", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = mons__GetSwitchState(soap, ret);
	if (soap->error)
		return soap->error;
	soap->encodingStyle = NULL;
	soap_serializeheader(soap);
	soap_serialize_mons__SwitchStateRes(soap, &ret);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_mons__SwitchStateRes(soap, &ret, "mons:SwitchStateRes", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_mons__SwitchStateRes(soap, &ret, "mons:SwitchStateRes", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetIngestSpeedLimit(struct soap *soap)
{	struct mons__GetIngestSpeedLimit soap_tmp_mons__GetIngestSpeedLimit;
	struct mons__IngestSpeedLimitRes ret;
	soap_default_mons__IngestSpeedLimitRes(soap, &ret);
	soap_default_mons__GetIngestSpeedLimit(soap, &soap_tmp_mons__GetIngestSpeedLimit);
	if (!soap_get_mons__GetIngestSpeedLimit(soap, &soap_tmp_mons__GetIngestSpeedLimit, "mons:GetIngestSpeedLimit", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = mons__GetIngestSpeedLimit(soap, ret);
	if (soap->error)
		return soap->error;
	soap->encodingStyle = NULL;
	soap_serializeheader(soap);
	soap_serialize_mons__IngestSpeedLimitRes(soap, &ret);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_mons__IngestSpeedLimitRes(soap, &ret, "mons:IngestSpeedLimitRes", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_mons__IngestSpeedLimitRes(soap, &ret, "mons:IngestSpeedLimitRes", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetWorkState_USCORECS(struct soap *soap)
{	struct mons__GetWorkState_USCORECS soap_tmp_mons__GetWorkState_USCORECS;
	struct mons__WorkStateRes ret;
	soap_default_mons__WorkStateRes(soap, &ret);
	soap_default_mons__GetWorkState_USCORECS(soap, &soap_tmp_mons__GetWorkState_USCORECS);
	if (!soap_get_mons__GetWorkState_USCORECS(soap, &soap_tmp_mons__GetWorkState_USCORECS, "mons:GetWorkState_CS", NULL))
		return soap->error;
	if (soap_body_end_in(soap)
	 || soap_envelope_end_in(soap)
	 || soap_end_recv(soap))
		return soap->error;
	soap->error = mons__GetWorkState_USCORECS(soap, ret);
	if (soap->error)
		return soap->error;
	soap->encodingStyle = NULL;
	soap_serializeheader(soap);
	soap_serialize_mons__WorkStateRes(soap, &ret);
	if (soap_begin_count(soap))
		return soap->error;
	if (soap->mode & SOAP_IO_LENGTH)
	{	if (soap_envelope_begin_out(soap)
		 || soap_putheader(soap)
		 || soap_body_begin_out(soap)
		 || soap_put_mons__WorkStateRes(soap, &ret, "mons:WorkStateRes", NULL)
		 || soap_body_end_out(soap)
		 || soap_envelope_end_out(soap))
			 return soap->error;
	};
	if (soap_end_count(soap)
	 || soap_response(soap, SOAP_OK)
	 || soap_envelope_begin_out(soap)
	 || soap_putheader(soap)
	 || soap_body_begin_out(soap)
	 || soap_put_mons__WorkStateRes(soap, &ret, "mons:WorkStateRes", NULL)
	 || soap_body_end_out(soap)
	 || soap_envelope_end_out(soap)
	 || soap_end_send(soap))
		return soap->error;
	return soap_closesock(soap);
}

#if defined(__BORLANDC__)
#pragma option pop
#pragma option pop
#endif

/* End of soapServer.cpp */
