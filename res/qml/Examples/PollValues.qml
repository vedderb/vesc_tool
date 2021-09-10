import QtQuick 2.5
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Controls.Material 2.2
import Vedder.vesc.utility 1.0
import Vedder.vesc.commands 1.0
import Vedder.vesc.configparams 1.0

Item {
    anchors.fill: parent
    anchors.margins: 10

    Material.theme: Utility.isDarkMode() ? "Dark" : "Light"
    Material.accent: Utility.getAppHexColor("lightAccent")
    
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
//            mCommands.bmsGetValues()
//            mCommands.ioBoardGetAll(255)
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
//            values.has_timeout
//            values.kill_sw_active
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
        
        onBmsValuesRx: { // val
            // Members of val
//            val.v_tot
//            val.v_charge
//            val.i_in
//            val.i_in_ic
//            val.ah_cnt
//            val.wh_cnt
//            val.v_cells[cell]
//            val.temps[sensor]
//            val.is_balancing[cell]
//            val.temp_ic
//            val.humidity
//            val.temp_hum_sensor
//            val.temp_cells_highest
//            val.soc
//            val.soh
//            val.can_id
        }
        
        onIoBoardValRx: {
            // Members of val
//            val.id
//            val.adc_1_4[ch]
//            val.adc_5_8[ch]
//            val.digital[ch]
//            val.adc_1_4_age
//            val.adc_5_8_age
//            val.digital_age
        }
    }
}
