(i2c-start)

; I2C Address
(def mpu-addr 0x68)

; MPU9250 registers
(def reg-pwr-mgmt1 0x6B)
(def reg-accel-config 0x1C)
(def reg-gyro-config 0x1B)
(def reg-accel-xout-h 0x3B)

; Clock source: Gyro X
(i2c-tx-rx mpu-addr (list reg-pwr-mgmt1 1))

; Accelerometer range: +-16 g
(i2c-tx-rx mpu-addr (list reg-accel-config (shl 0x03 3)))

; Gyro range: +-2000 deg/s
(i2c-tx-rx mpu-addr (list reg-gyro-config (shl 0x03 3)))

; Receive buffer for accel and gyro
(def rx-buf (array-create 14))

(loopwhile t {
        (i2c-tx-rx mpu-addr (list reg-accel-xout-h) rx-buf)
        
        (def acc-x (/ (* (bufget-i16 rx-buf 0) 16.0) 32768.0))
        (def acc-y (/ (* (bufget-i16 rx-buf 2) 16.0) 32768.0))
        (def acc-z (/ (* (bufget-i16 rx-buf 4) 16.0) 32768.0))
        
        (def gyro-x (/ (* (bufget-i16 rx-buf 8) 2000.0) 32768.0))
        (def gyro-y (/ (* (bufget-i16 rx-buf 10) 2000.0) 32768.0))
        (def gyro-z (/ (* (bufget-i16 rx-buf 12) 2000.0) 32768.0))
        
        (sleep 0.01)
})
