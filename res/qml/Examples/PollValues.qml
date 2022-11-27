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
//            mCommands.getStats(0xFFFFFFFF)
//            mCommands.getGnss(0xFFFF)
//            mCommands.getImuData(0xFFFFFFFF)
        }
    }
    
    Connections {
        target: mCommands
        
        function onValuesReceived(values, mask) {
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
        
        function onValuesSetupReceived(values, mask) {
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
        
        function onBmsValuesRx(val) {
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
        
        function onIoBoardValRx(val) {
            // Members of val
//            val.id
//            val.adc_1_4[ch]
//            val.adc_5_8[ch]
//            val.digital[ch]
//            val.adc_1_4_age
//            val.adc_5_8_age
//            val.digital_age
        }
        
        function onStatsRx(val, mask) {
            // Members of val
//            val.speed_avg
//            val.speed_max
//            val.power_avg
//            val.power_max
//            val.temp_motor_avg
//            val.temp_motor_max
//            val.temp_mos_avg
//            val.temp_mos_max
//            val.current_avg
//            val.current_max
//            val.count_time
//            val.distance() // Meters
//            val.energy() // Wh
//            val.efficiency() // Wh / km
//            val.ah()
        }
        
        function onGnssRx(val, mask) {
            // Members of val
//            val.lat
//            val.lon
//            val.height
//            val.speed
//            val.hdop
//            val.ms_today
//            val.yy
//            val.mo
//            val.dd
//            val.age_s
        }
        
        function onValuesImuReceived(val, mask) {
//            val.roll
//            val.pitch
//            val.yaw
//            val.accX
//            val.accY
//            val.accZ
//            val.gyroX
//            val.gyroY
//            val.gyroZ
//            val.magX
//            val.magY
//            val.magZ
//            val.q0
//            val.q1
//            val.q2
//            val.q3
//            val.vesc_id
        }
    }
}
