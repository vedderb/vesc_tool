import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0
import "qrc:/mobile"

Item {
    anchors.fill: parent
    anchors.margins: 10
    
    property Commands mCommands: VescIf.commands()
    property ConfigParams mMcConf: VescIf.mcConfig()
    property ConfigParams mAppConf: VescIf.appConfig()
    
    Timer {
        running: true
        repeat: true
        interval: 100
                
        onTriggered: {
            // Uncomment to poll. They will be received in the
            // corresponding connection below.
//            mCommands.getValues()
//            mCommands.getValuesSetup()
        }
    }
    
    Connections {
        target: mCommands
        
        onValuesReceived: { // values, mask
            // Members of values
//            values.v_in
//            values.temp_mos
//            values.temp_mos_1
//            values.temp_mos_2
//            values.temp_mos_3
//            values.temp_motor
//            values.current_motor
//            values.current_in
//            values.id
//            values.iq
//            values.rpm
//            values.duty_now
//            values.amp_hours
//            values.amp_hours_charged
//            values.watt_hours
//            values.watt_hours_charged
//            values.tachometer
//            values.tachometer_abs
//            values.position
//            values.fault_code
//            values.vesc_id
//            values.fault_str
//            values.vd
//            values.vq
        }
        
        onValuesSetupReceived: { // values, mask
            // Members of values
//            values.temp_mos
//            values.temp_motor
//            values.current_motor
//            values.current_in
//            values.duty_now
//            values.rpm
//            values.speed
//            values.v_in
//            values.battery_level
//            values.amp_hours
//            values.amp_hours_charged
//            values.watt_hours
//            values.watt_hours_charged
//            values.tachometer
//            values.tachometer_abs
//            values.position
//            values.fault_code
//            values.vesc_id
//            values.num_vescs
//            values.battery_wh
//            values.fault_str
//            values.odometer
        }
    }
}