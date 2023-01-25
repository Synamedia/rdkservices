/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include "HdmiCec.h"

#include "FactoriesImplementation.h"


#include "IarmBusMock.h"
#include "ServiceMock.h"
#include "devicesettings.h"
#include "LibCCECMock.h"
#include "ConnectionMock.h"
#include "LogicalAddressMock.h"
#include "ActiveSourceMock.h"
#include "DeviceTypeMock.h"
using namespace WPEFramework;

class HdmiCecTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::HdmiCec> plugin;
    Core::JSONRPC::Handler& handler;
    Core::JSONRPC::Connection connection;
    string response;

    HdmiCecTest()
        : plugin(Core::ProxyType<Plugin::HdmiCec>::Create())
        , handler(*(plugin))
        , connection(1, 0)
    {
    }
    virtual ~HdmiCecTest() = default;
};
class HdmiCecDsTest : public HdmiCecTest {
protected:
    LibCCECImplMock libCCECImplMock;
    ConnectionImplMock connectionImplMock;
    HdmiCecDsTest()
        : HdmiCecTest()
    {
        LibCCEC::getInstance().impl = &libCCECImplMock;
    }
    virtual ~HdmiCecDsTest() override
    {
        LibCCEC::getInstance().impl = nullptr;
    }
};

class HdmiCecInitializedTest : public HdmiCecTest {
protected:
    IarmBusImplMock iarmBusImplMock;
    IARM_EventHandler_t cecMgrEventHandler;
    IARM_EventHandler_t dsHdmiEventHandler;
 
    HdmiCecInitializedTest()
        : HdmiCecTest()
    {
        IarmBus::getInstance().impl = &iarmBusImplMock;

        ON_CALL(iarmBusImplMock, IARM_Bus_RegisterEventHandler(::testing::_, ::testing::_, ::testing::_))
            .WillByDefault(::testing::Invoke(
                [&](const char* ownerName, IARM_EventId_t eventId, IARM_EventHandler_t handler) {
                   if ((string(IARM_BUS_CECMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_CECMGR_EVENT_DAEMON_INITIALIZED)) {
                        EXPECT_TRUE(handler != nullptr);
                        cecMgrEventHandler = handler;
                    }
		   if ((string(IARM_BUS_CECMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_CECMGR_EVENT_STATUS_UPDATED)) {
                        EXPECT_TRUE(handler != nullptr);
                        cecMgrEventHandler = handler;
                    }
                   if ((string(IARM_BUS_DSMGR_NAME) == string(ownerName)) && (eventId == IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG)) {
                        EXPECT_TRUE(handler != nullptr);
                        dsHdmiEventHandler = handler;
                    }

                    return IARM_RESULT_SUCCESS;
                }));

        EXPECT_EQ(string(""), plugin->Initialize(nullptr));
    }
    virtual ~HdmiCecInitializedTest() override
    {

        sleep(2);
        plugin->Deinitialize(nullptr);

        IarmBus::getInstance().impl = nullptr;


    }
};
class HdmiCecInitializedEventTest : public HdmiCecInitializedTest {
protected:
    ServiceMock service;
    Core::JSONRPC::Message message;
    FactoriesImplementation factoriesImplementation;
    PluginHost::IDispatcher* dispatcher;

    HdmiCecInitializedEventTest()
        : HdmiCecInitializedTest()
    {
        PluginHost::IFactories::Assign(&factoriesImplementation);

        dispatcher = static_cast<PluginHost::IDispatcher*>(
            plugin->QueryInterface(PluginHost::IDispatcher::ID));
        dispatcher->Activate(&service);
    }

    virtual ~HdmiCecInitializedEventTest() override
    {
        dispatcher->Deactivate();
        dispatcher->Release();
        PluginHost::IFactories::Assign(nullptr);
    }
};
class HdmiCecInitializedEventDsTest : public HdmiCecInitializedEventTest {
protected:
    LibCCECImplMock libCCECImplMock;
    ConnectionImplMock connectionImplMock;
    DeviceTypeMock deviceTypeImplMock;
    LogicalAddressImplMock logicalAddressImplMock;
     

    HdmiCecInitializedEventDsTest()
        : HdmiCecInitializedEventTest()
    {
        LibCCEC::getInstance().impl = &libCCECImplMock;
        Connection::getInstance().impl = &connectionImplMock;	
	    DeviceType::getInstance().impl = &deviceTypeImplMock;
        LogicalAddress::getInstance().impl = &logicalAddressImplMock;

        

    }

    virtual ~HdmiCecInitializedEventDsTest() override
    {
        
        LibCCEC::getInstance().impl = nullptr;
        Connection::getInstance().impl = nullptr;
        DeviceType::getInstance().impl = nullptr;

    }
};


TEST_F(HdmiCecTest, RegisteredMethods)
{

    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("setEnabled")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getEnabled")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getCECAddresses")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendMessage")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getActiveSourceStatus")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("getDeviceList")));
    
}

TEST_F(HdmiCecInitializedEventDsTest, setEnabled)
{

    //setting HdmiCec to enabled.
    ON_CALL(libCCECImplMock, getLogicalAddress(::testing::_))
        .WillByDefault(::testing::Return(1));
	ON_CALL(deviceTypeImplMock, toString())
        .WillByDefault(::testing::Return("New"));
    EXPECT_CALL(connectionImplMock, open())
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&]() {
                
            }));
    EXPECT_CALL(connectionImplMock, addFrameListener(::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](FrameListener *listener) {
                //FrameListener only contains a destructor, and a void function. No values to check
            }));

	EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));

    //Turning off HdmiCec. otherwise we get segementation faults as things memory early while threads are still running
    sleep(1);//short wait to allow setEnabled to reach thread loop, where it can exit safely without segmentation faults

    EXPECT_CALL(connectionImplMock, close())
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&]() {
                
            }));
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}

TEST_F(HdmiCecDsTest, getEnabledFalse)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEnabled"), _T(""), response));
    EXPECT_EQ(response, string("{\"enabled\":false,\"success\":true}"));
}


TEST_F(HdmiCecInitializedEventDsTest, getEnabledTrue)
{
    //setting HdmiCec to enabled.
    ON_CALL(libCCECImplMock, getLogicalAddress(::testing::_))
        .WillByDefault(::testing::Return(1));
	ON_CALL(deviceTypeImplMock, toString())
        .WillByDefault(::testing::Return("New"));

	EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));

    //Get enabled just checks if CEC is on, which is a global variable.
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getEnabled"), _T(""), response));
    EXPECT_EQ(response, string("{\"enabled\":true,\"success\":true}"));



    //Turning off HdmiCec. otherwise we get segementation faults as things memory early while threads are still running
    sleep(1);//short wait to allow setEnabled to reach thread loop, where it can exit safely without segmentation faults
    ON_CALL(connectionImplMock, close())
        .WillByDefault(::testing::Return());
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}


TEST_F(HdmiCecInitializedEventDsTest, sendMessage)
{
    //setting HdmiCec to enabled.
    ON_CALL(libCCECImplMock, getLogicalAddress(::testing::_))
        .WillByDefault(::testing::Return(1));
	ON_CALL(deviceTypeImplMock, toString())
        .WillByDefault(::testing::Return("New"));
    ON_CALL(connectionImplMock, sendAsync(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](const CECFrame &frame) {
                EXPECT_EQ(frame.MAX_LENGTH, 128);
                
            }));
	EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));


    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendMessage"), _T("{\"message\": \"P4IwAA==\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));


    //Turning off HdmiCec. otherwise we get segementation faults as things memory early while threads are still running
    sleep(1);//short wait to allow setEnabled to reach thread loop, where it can exit safely without segmentation faults
    ON_CALL(connectionImplMock, close())
        .WillByDefault(::testing::Return());
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

}


TEST_F(HdmiCecInitializedEventDsTest, getActiveSourceStatusTrue)
{

    //setting HdmiCec to enabled.
    ON_CALL(libCCECImplMock, getLogicalAddress(::testing::_))
        .WillByDefault(::testing::Return(1));
	ON_CALL(deviceTypeImplMock, toString())
        .WillByDefault(::testing::Return("xi6"));
    
	EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));

    //sendMessage() with the proper message sets ActiveSource to true.
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendMessage"), _T("{\"message\": \"P4IwAA==\"}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));


    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T(""), response));
    EXPECT_EQ(response, string("{\"status\":true,\"success\":true}"));


     //Turning off HdmiCec. otherwise we get segementation faults as things memory early while threads are still running
    sleep(1);//short wait to allow setEnabled to reach thread loop, where it can exit safely without segmentation faults
    ON_CALL(connectionImplMock, close())
        .WillByDefault(::testing::Return());
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));


}
TEST_F(HdmiCecInitializedEventDsTest, getActiveSourceStatusFalse)
{

    //setting HdmiCec to enabled.
    ON_CALL(libCCECImplMock, getLogicalAddress(::testing::_))
        .WillByDefault(::testing::Return(1));
	ON_CALL(deviceTypeImplMock, toString())
        .WillByDefault(::testing::Return("New"));

    //Not using sendMessage, such that active source is set to false, as it is by default)
    
	EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));


    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getActiveSourceStatus"), _T(""), response));
    EXPECT_EQ(response, string("{\"status\":false,\"success\":true}"));


     //Turning off HdmiCec. otherwise we get segementation faults as things memory early while threads are still running
    sleep(1);//short wait to allow setEnabled to reach thread loop, where it can exit safely without segmentation faults
    ON_CALL(connectionImplMock, close())
        .WillByDefault(::testing::Return());
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));


}


TEST_F(HdmiCecInitializedEventDsTest, getCECAddress)
{
    //setting HdmiCec to enabled.
	EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));



    ON_CALL(libCCECImplMock, getPhysicalAddress(::testing::_))
        .WillByDefault(::testing::Invoke(
            [&](uint32_t *physAddress) {
                *physAddress = (uint32_t)12345;
            }));
    ASSERT_TRUE(cecMgrEventHandler != nullptr);
    IARM_Bus_CECMgr_Status_Updated_Param_t eventData;

    //Active Source Status update sets the address/logical address and what not to non-default values
    eventData.logicalAddress =42;
    
    handler.Subscribe(0, _T("onActiveSourceStatusUpdate"), _T("client.events.onActiveSourceStatusUpdate"), message);

	ON_CALL(deviceTypeImplMock, toString())
        .WillByDefault(::testing::Return("New"));
    cecMgrEventHandler(IARM_BUS_CECMGR_NAME, IARM_BUS_CECMGR_EVENT_STATUS_UPDATED, &eventData , 0);


    handler.Unsubscribe(0, _T("onActiveSourceStatusUpdate"), _T("client.events.onActiveSourceStatusUpdate"), message);


    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getCECAddresses"), _T(""), response));
    EXPECT_EQ(response, string("{\"CECAddresses\":{\"physicalAddress\":12345,\"logicalAddress\":42,\"deviceType\":\"New\"},\"success\":true}"));




    //Turning off HdmiCec. otherwise we get segementation faults as things memory early while threads are still running
    sleep(1);//short wait to allow setEnabled to reach thread loop, where it can exit safely without segmentation faults
    ON_CALL(connectionImplMock, close())
        .WillByDefault(::testing::Return());
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));
}



TEST_F(HdmiCecInitializedEventDsTest, onDevicesChanged)
{
 //setting HdmiCec to enabled.
    ON_CALL(libCCECImplMock, getLogicalAddress(::testing::_))
        .WillByDefault(::testing::Return(1));
	ON_CALL(deviceTypeImplMock, toString())
        .WillByDefault(::testing::Return("New"));
    
	EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));


    ASSERT_TRUE(dsHdmiEventHandler != nullptr);


    IARM_Bus_DSMgr_EventData_t eventData;
    eventData.data.hdmi_in_connect.port =dsHDMI_IN_PORT_0;
    eventData.data.hdmi_in_connect.isPortConnected = true;

    handler.Subscribe(0, _T("onDeviceAdded"), _T("client.events.onDeviceAdded"), message);

    dsHdmiEventHandler(IARM_BUS_DSMGR_NAME, IARM_BUS_DSMGR_EVENT_HDMI_HOTPLUG, &eventData , 0);

    handler.Unsubscribe(0, _T("onDeviceAdded"), _T("client.events.onDeviceAdded"), message);

    //Turning off HdmiCec. otherwise we get segementation faults as things memory early while threads are still running
    sleep(1);//short wait to allow setEnabled to reach thread loop, where it can exit safely without segmentation faults
    ON_CALL(connectionImplMock, close())
        .WillByDefault(::testing::Return());
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));


}

TEST_F(HdmiCecInitializedEventDsTest, getDeviceList)
{

     //setting HdmiCec to enabled.
    ON_CALL(libCCECImplMock, getLogicalAddress(::testing::_))
        .WillByDefault(::testing::Return(1));
	ON_CALL(deviceTypeImplMock, toString())
        .WillByDefault(::testing::Return("New"));
    
	EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": true}"), response));
        EXPECT_EQ(response, string("{\"success\":true}"));
    sleep(1); //Allow the thread that populates deviceList to actually populate before we run getDeviceList.

    //Calling the device list, which is a defualt list of the hdmiCec class. Kist grabs the deviceList.
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("getDeviceList"), _T(""), response));
    EXPECT_THAT(response, ::testing::ContainsRegex(_T(".*[({\"logicalAddress\":[0-9]*,\"osdName\":\"[a-zA-Z0-9 ]*\",\"vendorID\":\"[a-zA-Z0-9 ]*\"})*.*")));
    EXPECT_THAT(response, ::testing::ContainsRegex(_T(".*\"numberofdevices\":[0-9]*,\"deviceList\":.*")));
    EXPECT_THAT(response, ::testing::ContainsRegex(_T(".*\"success\":true.*")));


    //Turning off HdmiCec. otherwise we get segementation faults as things memory early while threads are still running
    sleep(1);//short wait to allow setEnabled to reach thread loop, where it can exit safely without segmentation faults
    ON_CALL(connectionImplMock, close())
        .WillByDefault(::testing::Return());
    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("setEnabled"), _T("{\"enabled\": false}"), response));
    EXPECT_EQ(response, string("{\"success\":true}"));

}
