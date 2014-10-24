/* soapStub.h
   Generated by gSOAP 2.7.15 from ./webservice.h
   Copyright(C) 2000-2009, Robert van Engelen, Genivia Inc. All Rights Reserved.
   This part of the software is released under one of the following licenses:
   GPL, the gSOAP public license, or Genivia's license for commercial use.
*/

#ifndef soapStub_H
#define soapStub_H
#include <vector>
#include "stdsoap2.h"

/******************************************************************************\
 *                                                                            *
 * Enumerations                                                               *
 *                                                                            *
\******************************************************************************/


/******************************************************************************\
 *                                                                            *
 * Types with Custom Serializers                                              *
 *                                                                            *
\******************************************************************************/


/******************************************************************************\
 *                                                                            *
 * Classes and Structs                                                        *
 *                                                                            *
\******************************************************************************/


#if 0 /* volatile type: do not redeclare here */

#endif

#ifndef SOAP_TYPE_mons__MontorStateRes
#define SOAP_TYPE_mons__MontorStateRes (8)
/* mons:MontorStateRes */
struct mons__MontorStateRes
{
public:
	int bMain;	/* SOAP 1.2 RPC return element (when namespace qualified) */	/* required element of type xsd:int */
	int iState;	/* required element of type xsd:int */
};
#endif

#ifndef SOAP_TYPE_mons__GetMontorState
#define SOAP_TYPE_mons__GetMontorState (11)
/* mons:GetMontorState */
struct mons__GetMontorState
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#ifndef SOAP_TYPE_mons__TmsStateRes
#define SOAP_TYPE_mons__TmsStateRes (12)
/* mons:TmsStateRes */
struct mons__TmsStateRes
{
public:
	int bRun;	/* SOAP 1.2 RPC return element (when namespace qualified) */	/* required element of type xsd:int */
	int iWorkState;	/* required element of type xsd:int */
	int iState;	/* required element of type xsd:int */
};
#endif

#ifndef SOAP_TYPE_mons__GetTMSState
#define SOAP_TYPE_mons__GetTMSState (15)
/* mons:GetTMSState */
struct mons__GetTMSState
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#if 0 /* volatile type: do not redeclare here */

#endif

#ifndef SOAP_TYPE_mons__SMSState
#define SOAP_TYPE_mons__SMSState (16)
/* mons:SMSState */
struct mons__SMSState
{
public:
	std::string HallId;	/* SOAP 1.2 RPC return element (when namespace qualified) */	/* required element of type xsd:string */
	int bRun;	/* required element of type xsd:int */
	int state;	/* required element of type xsd:int */
	int position;	/* required element of type xsd:int */
	std::string strSplUuid;	/* required element of type xsd:string */
};
#endif

#ifndef SOAP_TYPE_mons__GetSMSState
#define SOAP_TYPE_mons__GetSMSState (20)
/* mons:GetSMSState */
struct mons__GetSMSState
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#ifndef SOAP_TYPE_mons__RaidStateRes
#define SOAP_TYPE_mons__RaidStateRes (21)
/* mons:RaidStateRes */
struct mons__RaidStateRes
{
public:
	int state;	/* SOAP 1.2 RPC return element (when namespace qualified) */	/* required element of type xsd:int */
	int ReadSpeed;	/* required element of type xsd:int */
	int WriteSpeed;	/* required element of type xsd:int */
	std::vector<int >diskState;	/* optional element of type xsd:int */
};
#endif

#ifndef SOAP_TYPE_mons__GetRaidtate
#define SOAP_TYPE_mons__GetRaidtate (25)
/* mons:GetRaidtate */
struct mons__GetRaidtate
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#ifndef SOAP_TYPE_mons__ethstate
#define SOAP_TYPE_mons__ethstate (26)
/* mons:ethstate */
struct mons__ethstate
{
public:
	int eth;	/* required element of type xsd:int */
	int type;	/* required element of type xsd:int */
	int ConnectState;	/* required element of type xsd:int */
	int speed;	/* required element of type xsd:int */
};
#endif

#ifndef SOAP_TYPE_mons__GetEthStateResponse
#define SOAP_TYPE_mons__GetEthStateResponse (30)
/* mons:GetEthStateResponse */
struct mons__GetEthStateResponse
{
public:
	std::vector<struct mons__ethstate >vecret;	/* SOAP 1.2 RPC return element (when namespace qualified) */	/* required element of type mons:ethstate */
};
#endif

#ifndef SOAP_TYPE_mons__GetEthState
#define SOAP_TYPE_mons__GetEthState (31)
/* mons:GetEthState */
struct mons__GetEthState
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#ifndef SOAP_TYPE_mons__SwitchStateRes
#define SOAP_TYPE_mons__SwitchStateRes (32)
/* mons:SwitchStateRes */
struct mons__SwitchStateRes
{
public:
	int Switch1State;	/* SOAP 1.2 RPC return element (when namespace qualified) */	/* required element of type xsd:int */
	int Switch2State;	/* required element of type xsd:int */
};
#endif

#ifndef SOAP_TYPE_mons__GetSwitchState
#define SOAP_TYPE_mons__GetSwitchState (35)
/* mons:GetSwitchState */
struct mons__GetSwitchState
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#ifndef SOAP_TYPE_mons__IngestSpeedLimitRes
#define SOAP_TYPE_mons__IngestSpeedLimitRes (36)
/* mons:IngestSpeedLimitRes */
struct mons__IngestSpeedLimitRes
{
public:
	int bEnableIngest;	/* SOAP 1.2 RPC return element (when namespace qualified) */	/* required element of type xsd:int */
	int speedLimit;	/* required element of type xsd:int */
};
#endif

#ifndef SOAP_TYPE_mons__GetIngestSpeedLimit
#define SOAP_TYPE_mons__GetIngestSpeedLimit (39)
/* mons:GetIngestSpeedLimit */
struct mons__GetIngestSpeedLimit
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#ifndef SOAP_TYPE_mons__WorkStateRes
#define SOAP_TYPE_mons__WorkStateRes (40)
/* mons:WorkStateRes */
struct mons__WorkStateRes
{
public:
	int state;	/* SOAP 1.2 RPC return element (when namespace qualified) */	/* required element of type xsd:int */
	std::string info;	/* required element of type xsd:string */
	std::string hall;	/* required element of type xsd:string */
};
#endif

#ifndef SOAP_TYPE_mons__GetWorkState_USCORECS
#define SOAP_TYPE_mons__GetWorkState_USCORECS (43)
/* mons:GetWorkState_CS */
struct mons__GetWorkState_USCORECS
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#ifndef SOAP_TYPE_mons__ExeSwitchTMSToOtherResponse
#define SOAP_TYPE_mons__ExeSwitchTMSToOtherResponse (46)
/* mons:ExeSwitchTMSToOtherResponse */
struct mons__ExeSwitchTMSToOtherResponse
{
public:
	int ret;	/* SOAP 1.2 RPC return element (when namespace qualified) */	/* required element of type xsd:int */
};
#endif

#ifndef SOAP_TYPE_mons__ExeSwitchTMSToOther
#define SOAP_TYPE_mons__ExeSwitchTMSToOther (47)
/* mons:ExeSwitchTMSToOther */
struct mons__ExeSwitchTMSToOther
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#ifndef SOAP_TYPE_SOAP_ENV__Header
#define SOAP_TYPE_SOAP_ENV__Header (48)
/* SOAP Header: */
struct SOAP_ENV__Header
{
#ifdef WITH_NOEMPTYSTRUCT
private:
	char dummy;	/* dummy member to enable compilation */
#endif
};
#endif

#ifndef SOAP_TYPE_SOAP_ENV__Code
#define SOAP_TYPE_SOAP_ENV__Code (49)
/* SOAP Fault Code: */
struct SOAP_ENV__Code
{
public:
	char *SOAP_ENV__Value;	/* optional element of type xsd:QName */
	struct SOAP_ENV__Code *SOAP_ENV__Subcode;	/* optional element of type SOAP-ENV:Code */
};
#endif

#ifndef SOAP_TYPE_SOAP_ENV__Detail
#define SOAP_TYPE_SOAP_ENV__Detail (51)
/* SOAP-ENV:Detail */
struct SOAP_ENV__Detail
{
public:
	int __type;	/* any type of element <fault> (defined below) */
	void *fault;	/* transient */
	char *__any;
};
#endif

#ifndef SOAP_TYPE_SOAP_ENV__Reason
#define SOAP_TYPE_SOAP_ENV__Reason (54)
/* SOAP-ENV:Reason */
struct SOAP_ENV__Reason
{
public:
	char *SOAP_ENV__Text;	/* optional element of type xsd:string */
};
#endif

#ifndef SOAP_TYPE_SOAP_ENV__Fault
#define SOAP_TYPE_SOAP_ENV__Fault (55)
/* SOAP Fault: */
struct SOAP_ENV__Fault
{
public:
	char *faultcode;	/* optional element of type xsd:QName */
	char *faultstring;	/* optional element of type xsd:string */
	char *faultactor;	/* optional element of type xsd:string */
	struct SOAP_ENV__Detail *detail;	/* optional element of type SOAP-ENV:Detail */
	struct SOAP_ENV__Code *SOAP_ENV__Code;	/* optional element of type SOAP-ENV:Code */
	struct SOAP_ENV__Reason *SOAP_ENV__Reason;	/* optional element of type SOAP-ENV:Reason */
	char *SOAP_ENV__Node;	/* optional element of type xsd:string */
	char *SOAP_ENV__Role;	/* optional element of type xsd:string */
	struct SOAP_ENV__Detail *SOAP_ENV__Detail;	/* optional element of type SOAP-ENV:Detail */
};
#endif

/******************************************************************************\
 *                                                                            *
 * Typedefs                                                                   *
 *                                                                            *
\******************************************************************************/

#ifndef SOAP_TYPE__QName
#define SOAP_TYPE__QName (5)
typedef char *_QName;
#endif

#ifndef SOAP_TYPE__XML
#define SOAP_TYPE__XML (6)
typedef char *_XML;
#endif


/******************************************************************************\
 *                                                                            *
 * Externals                                                                  *
 *                                                                            *
\******************************************************************************/


/******************************************************************************\
 *                                                                            *
 * Service Operations                                                         *
 *                                                                            *
\******************************************************************************/


SOAP_FMAC5 int SOAP_FMAC6 mons__GetMontorState(struct soap*, struct mons__MontorStateRes &ret);

SOAP_FMAC5 int SOAP_FMAC6 mons__GetTMSState(struct soap*, struct mons__TmsStateRes &ret);

SOAP_FMAC5 int SOAP_FMAC6 mons__GetSMSState(struct soap*, struct mons__SMSState &ret);

SOAP_FMAC5 int SOAP_FMAC6 mons__GetRaidtate(struct soap*, struct mons__RaidStateRes &ret);

SOAP_FMAC5 int SOAP_FMAC6 mons__GetEthState(struct soap*, std::vector<struct mons__ethstate >&vecret);

SOAP_FMAC5 int SOAP_FMAC6 mons__GetSwitchState(struct soap*, struct mons__SwitchStateRes &ret);

SOAP_FMAC5 int SOAP_FMAC6 mons__GetIngestSpeedLimit(struct soap*, struct mons__IngestSpeedLimitRes &ret);

SOAP_FMAC5 int SOAP_FMAC6 mons__GetWorkState_USCORECS(struct soap*, struct mons__WorkStateRes &ret);

SOAP_FMAC5 int SOAP_FMAC6 mons__ExeSwitchTMSToOther(struct soap*, int &ret);

/******************************************************************************\
 *                                                                            *
 * Skeletons                                                                  *
 *                                                                            *
\******************************************************************************/

SOAP_FMAC5 int SOAP_FMAC6 soap_serve(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_request(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetMontorState(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetTMSState(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetSMSState(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetRaidtate(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetEthState(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetSwitchState(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetIngestSpeedLimit(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__GetWorkState_USCORECS(struct soap*);

SOAP_FMAC5 int SOAP_FMAC6 soap_serve_mons__ExeSwitchTMSToOther(struct soap*);

#endif

/* End of soapStub.h */
