// 
// Copyright (C) 2005 SIPfoundry Inc.
// Licensed by SIPfoundry under the LGPL license.
// 
// Copyright (C) 2005 Pingtel Corp.
// Licensed to SIPfoundry under a Contributor Agreement.
// 
// $$
//////////////////////////////////////////////////////////////////////////////

// SYSTEM INCLUDES

// APPLICATION INCLUDES
#include <utl/UtlInt.h>
#include <utl/UtlBool.h>
#include <utl/UtlDateTime.h>
#include <utl/UtlSListIterator.h>
#include <utl/UtlHashMapIterator.h>
#include <os/OsSysLog.h>
#include <xmlparser/tinyxml.h>
#include "net/XmlRpcResponse.h"

// EXTERNAL FUNCTIONS
// EXTERNAL VARIABLES
// CONSTANTS
// STATIC VARIABLE INITIALIZATIONS

/* //////////////////////////// PUBLIC //////////////////////////////////// */

/* ============================ CREATORS ================================== */

// Constructor
XmlRpcResponse::XmlRpcResponse()
{
   mFaultCode = ILL_FORMED_CONTENTS_FAULT_CODE;
   mFaultString = ILL_FORMED_CONTENTS_FAULT_STRING;

   // Start to construct the XML-RPC body
   mpResponseBody = new XmlRpcBody();
}

// Copy constructor
XmlRpcResponse::XmlRpcResponse(const XmlRpcResponse& rXmlRpcResponse)
{
}

// Destructor
XmlRpcResponse::~XmlRpcResponse()
{
}

bool XmlRpcResponse::parseXmlRpcResponse(UtlString& responseContent)
{
   bool result = false;
   
   // Parse the XML-RPC response
   TiXmlDocument doc("XmlRpcResponse.xml");      
   if (doc.Parse(responseContent))
   {
      TiXmlNode* rootNode = doc.FirstChild ("methodResponse");      
      if (rootNode != NULL)
      {
         // Positive response example
         // 
         // <methodResponse> 
         //   <params>
         //     <param>
         //       <value><string>South Dakota</string></value>
         //     </param>
         //   </params>
         // </methodResponse>
         
         TiXmlNode* paramsNode = rootNode->FirstChild("params");        
         if (paramsNode != NULL)
         {
            TiXmlNode* paramNode = paramsNode->FirstChild("param");
            if (paramNode)
            {
               TiXmlNode* subNode = paramNode->FirstChild("value");              
               if (subNode)
               {
                  result = parseValue(subNode);
               }
            }
         }
         else
         {
            // Fault response example
            //
            // <methodResponse>
            //   <fault>
            //     <value>
            //       <struct>
            //         <member>
            //           <name>faultCode</name>
            //           <value><int>4</int></value>
            //         </member>
            //         <member>
            //           <name>faultString</name>
            //           <value><string>Too many parameters.</string></value>
            //         </member>
            //       </struct>
            //     </value>
            //   </fault>
            // </methodResponse>
            
            TiXmlNode* faultNode = rootNode->FirstChild("fault");
            
            if (faultNode != NULL)
            {
               TiXmlNode* subNode = faultNode->FirstChild("value");
               
               if (subNode != NULL)
               {
                  subNode = subNode->FirstChild("struct");
                  
                  if (subNode != NULL)
                  {
                     for (TiXmlNode* memberNode = subNode->FirstChild("member");
                          memberNode; 
                          memberNode = memberNode->NextSibling("member"))
                     {
                        UtlString nameValue;
                        nameValue = (memberNode->FirstChild("name"))->FirstChild()->Value();
                        
                        if (nameValue.compareTo("faultCode") == 0)
                        {
                           TiXmlNode* valueNode = (memberNode->FirstChild("value"))->FirstChild("int");
                           if (valueNode != NULL)
                           {
                              mFaultCode = atoi(valueNode->FirstChild()->Value());
                           }
                           
                           valueNode = (memberNode->FirstChild("value"))->FirstChild("i4");
                           if (valueNode != NULL)
                           {
                              mFaultCode = atoi(valueNode->FirstChild()->Value());
                           }
                        }
                           
                        if (nameValue.compareTo("faultString") == 0)
                        {
                           TiXmlNode* valueNode = (memberNode->FirstChild("value"))->FirstChild("string");
                           if (valueNode != NULL)
                           {
                              mFaultString = valueNode->FirstChild()->Value();
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   
   return result;  
}

/* ============================ MANIPULATORS ============================== */


// Assignment operator
XmlRpcResponse&
XmlRpcResponse::operator=(const XmlRpcResponse& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;
}

/* ============================ ACCESSORS ================================= */

bool XmlRpcResponse::setResponse(UtlContainable* value)
{
   bool result = false;
   mpResponseBody->append(BEGIN_RESPONSE);   
   mpResponseBody->append(BEGIN_PARAMS);   
   mpResponseBody->append(BEGIN_PARAM);  
   
   result = mpResponseBody->addValue(value);        
   
   mpResponseBody->append(END_PARAM);
   mpResponseBody->append(END_PARAMS);   
   mpResponseBody->append(END_RESPONSE);   
        
   UtlString bodyString;
   int bodyLength;
   mpResponseBody->getBytes(&bodyString, &bodyLength);
   OsSysLog::add(FAC_SIP, PRI_DEBUG,
                 "mpResponseBody::setResponse XML-RPC response message = \n%s\n", bodyString.data());
   return result;
}


bool XmlRpcResponse::setFault(int faultCode, const char* faultString)
{
   bool result = true;
   mFaultCode = faultCode;
   mFaultString = faultString;

   // Start to construct the XML-RPC body for fault response
   if (mpResponseBody)
   {
      delete mpResponseBody;
      mpResponseBody = NULL;
   }

   mpResponseBody = new XmlRpcBody();

   // Fault response example
   //
   // <methodResponse>
   //   <fault>
   //     <value>
   //       <struct>
   //         <member>
   //           <name>faultCode</name>
   //           <value><int>4</int></value>
   //         </member>
   //         <member>
   //           <name>faultString</name>
   //           <value><string>Too many parameters.</string></value>
   //         </member>
   //       </struct>
   //     </value>
   //   </fault>
   // </methodResponse>

   mpResponseBody->append(BEGIN_RESPONSE);   
   mpResponseBody->append(BEGIN_FAULT);   
   mpResponseBody->append(BEGIN_STRUCT);
      
   mpResponseBody->append(BEGIN_MEMBER);   
   mpResponseBody->append(FAULT_CODE);
   
   char temp[10];
   sprintf(temp, "%d", mFaultCode);
   UtlString paramValue = BEGIN_INT + UtlString(temp) + END_INT;
   mpResponseBody->append(paramValue);
   
   mpResponseBody->append(END_MEMBER);   
   
   mpResponseBody->append(BEGIN_MEMBER);   
   mpResponseBody->append(FAULT_STRING);
   
   paramValue = BEGIN_STRING + mFaultString + END_STRING;
   mpResponseBody->append(paramValue);
   
   mpResponseBody->append(END_MEMBER);   
      
   mpResponseBody->append(END_STRUCT);   
   mpResponseBody->append(END_FAULT);   
   mpResponseBody->append(END_RESPONSE);
      
   UtlString bodyString;
   int bodyLength;
   mpResponseBody->getBytes(&bodyString, &bodyLength);
   OsSysLog::add(FAC_SIP, PRI_DEBUG,
                 "mpResponseBody::setFault XML-RPC response message = \n%s\n", bodyString.data());

   return result;
}


bool XmlRpcResponse::getResponse(UtlContainable*& value)
{
   bool result = false;
   
   value = mResponseValue;
   
   if (value)
   {
      result = true;
   }
   else
   {
      result = false;
   }
   
   return result;
}


void XmlRpcResponse::getFault(int* faultCode, UtlString& faultString)
{
   *faultCode = mFaultCode;
   faultString = mFaultString;
}


/* ============================ INQUIRY =================================== */

/* //////////////////////////// PROTECTED ///////////////////////////////// */

/* //////////////////////////// PRIVATE /////////////////////////////////// */
bool XmlRpcResponse::parseValue(TiXmlNode* subNode)
{
   bool result = false;

   UtlString paramValue;
                  
   // four-byte signed integer
   TiXmlNode* valueNode = subNode->FirstChild("i4");                  
   if (valueNode)
   {
      paramValue = valueNode->FirstChild()->Value();
      mResponseValue = new UtlInt(atoi(paramValue));
      result = true;
   }

   valueNode = subNode->FirstChild("int");                  
   if (valueNode)
   {
      paramValue = valueNode->FirstChild()->Value();
      mResponseValue = new UtlInt(atoi(paramValue));
      result = true;
   }

   // boolean
   valueNode = subNode->FirstChild("boolean");                 
   if (valueNode)
   {
      paramValue = valueNode->FirstChild()->Value();
      mResponseValue = new UtlBool((atoi(paramValue)==1));
      result = true;
   }
                  
   // string
   valueNode = subNode->FirstChild("string");                  
   if (valueNode)
   {
      paramValue = valueNode->FirstChild()->Value();
      mResponseValue = new UtlString(paramValue);
      result = true;
   }
                  
   // dateTime.iso8601
   valueNode = subNode->FirstChild("dateTime.iso8601");                 
   if (valueNode)
   {
      paramValue = valueNode->FirstChild()->Value();
      //mResponseValue = new UtlDateTime(paramValue);
      result = true;
   }
                  
   // struct
   valueNode = subNode->FirstChild("struct");                  
   if (valueNode)
   {
      UtlHashMap* map = new UtlHashMap();
      parseStruct(valueNode, map);
      mResponseValue = map;
      result = true;
   }

   // array
   valueNode = subNode->FirstChild("array");                  
   if (valueNode)
   {
      UtlSList* list = new UtlSList();
      parseArray(valueNode, list);
      mResponseValue = list;
      result = true;
   }
   
   return result;
}
   
bool XmlRpcResponse::parseStruct(TiXmlNode* subNode, UtlHashMap* members)
{
   bool result = true;

   // struct
   UtlString name;
   UtlString paramValue;
   TiXmlNode* memberValue;
   for (TiXmlNode* memberNode = subNode->FirstChild("member");
        memberNode; 
        memberNode = memberNode->NextSibling("member"))
   {
      TiXmlNode* memberName = memberNode->FirstChild("name");
      if (memberName)
      {
         name = memberName->FirstChild()->Value();
         
         memberValue = memberNode->FirstChild("value");        
         if (memberValue)
         {
            // four-byte signed integer                         
            TiXmlNode* valueElement = memberValue->FirstChild("i4");
            if (valueElement)
            {
               paramValue = valueElement->FirstChild()->Value();
               members->insertKeyAndValue(new UtlString(name), new UtlInt(atoi(paramValue)));
            }
            else
            {
               valueElement = memberValue->FirstChild("int");
               if (valueElement)
               {
                  paramValue = valueElement->FirstChild()->Value();
                  members->insertKeyAndValue(new UtlString(name), new UtlInt(atoi(paramValue)));
               }
               else
               {
                  valueElement = memberValue->FirstChild("boolean");
                  if (valueElement)
                  {
                     paramValue = valueElement->FirstChild()->Value();
                     members->insertKeyAndValue(new UtlString(name), new UtlBool((atoi(paramValue)==1)));
                  }
                  else
                  {              
                     valueElement = memberValue->FirstChild("string");
                     if (valueElement)
                     {
                        paramValue = valueElement->FirstChild()->Value();
                        members->insertKeyAndValue(new UtlString(name), new UtlString(paramValue));
                     }
                     else
                     {
                        valueElement = memberValue->FirstChild("dateTime.iso8601");
                        if (valueElement)
                        {
                           paramValue = valueElement->FirstChild()->Value();
                           members->insertKeyAndValue(new UtlString(name), new UtlString(paramValue));
                        }
                        else
                        {
                           valueElement = memberValue->FirstChild("struct");
                           if (valueElement)
                           {
                              UtlHashMap* members = new UtlHashMap();
                              parseStruct(valueElement, members);
                              members->insertKeyAndValue(new UtlString(name), members);
                           }
                           else
                           {
                              valueElement = memberValue->FirstChild("array");
                              if (valueElement)
                              {
                                 UtlSList* subArray = new UtlSList();
                                 parseArray(valueElement, subArray);
                                 members->insertKeyAndValue(new UtlString(name), subArray);
                              }
                              else
                              {
                                 // default for string
                                 paramValue = memberValue->FirstChild()->Value();
                                 members->insertKeyAndValue(new UtlString(name), new UtlString(paramValue));
                              }
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   
   return result;   
}

bool XmlRpcResponse::parseArray(TiXmlNode* subNode, UtlSList* array)
{
   bool result = true;
   
   // array
   UtlString paramValue;
   TiXmlNode* dataNode = subNode->FirstChild("data");
   if (dataNode)
   {
      for (TiXmlNode* valueNode = dataNode->FirstChild("value");
           valueNode; 
           valueNode = valueNode->NextSibling("value"))
      {
         // four-byte signed integer                         
         TiXmlNode* arrayElement = valueNode->FirstChild("i4");
         if (arrayElement)
         {
            paramValue = arrayElement->FirstChild()->Value();
            array->insert(new UtlInt(atoi(paramValue)));
         }
         else
         {
            arrayElement = valueNode->FirstChild("int");
            if (arrayElement)
            {
               paramValue = arrayElement->FirstChild()->Value();
               array->insert(new UtlInt(atoi(paramValue)));
            }
            else
            {
               arrayElement = valueNode->FirstChild("boolean");
               if (arrayElement)
               {
                  paramValue = arrayElement->FirstChild()->Value();
                  array->insert(new UtlBool((atoi(paramValue)==1)));
               }
               else
               {              
                  arrayElement = valueNode->FirstChild("string");
                  if (arrayElement)
                  {
                     paramValue = arrayElement->FirstChild()->Value();
                     array->insert(new UtlString(paramValue));
                  }
                  else
                  {
                     arrayElement = valueNode->FirstChild("dateTime.iso8601");
                     if (arrayElement)
                     {
                        paramValue = arrayElement->FirstChild()->Value();
                        array->insert(new UtlString(paramValue));
                     }
                     else
                     {
                        arrayElement = valueNode->FirstChild("struct");
                        if (arrayElement)
                        {
                           UtlHashMap* members = new UtlHashMap();
                           parseStruct(arrayElement, members);
                           array->insert(members);
                        }
                        else
                        {
                           arrayElement = valueNode->FirstChild("array");
                           if (arrayElement)
                           {
                              UtlSList* subArray = new UtlSList();
                              parseArray(arrayElement, subArray);
                              array->insert(subArray);
                           }
                           else
                           {
                              // default for string
                              paramValue = valueNode->FirstChild()->Value();
                              array->insert(new UtlString(paramValue));
                           }
                        }
                     }
                  }
               }
            }
         }
      }
   }
   
   return result;
}

/* ============================ FUNCTIONS ================================= */

