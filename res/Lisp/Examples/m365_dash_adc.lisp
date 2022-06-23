; M365 dashboard compability lisp script by Netzpfuscher
; Wiring: red=5V black=GND yellow=COM-TX (UART-HDX) green=COM-RX (button)+3.3V with R470 Resistor

(app-adc-detach 3 1) ; Detach ADC1/2 and cc/rev button from APP

; **** User parameters ****
(define light-default 0)
(define show-faults 1)
(define show-batt-in-idle 1)
(define min-speed 1)

; Speed modes with MAX KM/H and MAX WATTS
(define speed-eco 7)
(define watt-eco 400)
(define speed-drive 17)
(define watt-drive 500)
(define speed-sport 21) ; or 400
(define watt-sport 700) ; or 1500000

; **** Code section ****
(uart-start 115200 'half-duplex)
(gpio-configure 'pin-rx 'pin-mode-in-pu)

(define tx-frame (array-create 14))
(bufset-u16 tx-frame 0 0x55AA)
(bufset-u16 tx-frame 2 0x0821)
(bufset-u16 tx-frame 4 0x6400)

(define uart-buf (array-create type-byte 64))
(define throttle 0)
(define brake 0)
(define buttonold 0)
(define light 0)
(setvar 'light light-default)
(define c-out 0)
(define code 0)

; Button handling

(define presstime (systime))
(define presses 0)

; Mode states

(define off 0)
(define lock 0)
(define speedmode 4)

(conf-set 'max-speed (/ speed-sport 3.6))
(conf-set 'l-watt-max watt-sport)

; Sound feedback

(define feedback 0)

(defun inp(buffer) ; Frame 0x65
    (progn
        (setvar 'throttle (/(bufget-u8 uart-buf 4) 255.0))
        (setvar 'brake (/(bufget-u8 uart-buf 5) 255.0))

        (if (= (+ off lock) 0)
            (progn
                (app-adc-override 0 throttle)
                (if (= lock 1)
                    (app-adc-override 1 1)
                    (app-adc-override 1 brake)
                )
            )
            (progn
                (app-adc-override 0 0)
                (app-adc-override 1 0)
            )
        )
    )
)

(defun outp(buffer) ; Frame 0x64
    (progn
        ; mode field (1=drive, 2=eco, 4=sport, 8=charge, 16=off, 32=lock)
        (if (= off 1)
            (bufset-u8 tx-frame 6 16)
            (if (= lock 1)
                (bufset-u8 tx-frame 6 32) ; lock display
                (bufset-u8 tx-frame 6 speedmode)
            )
        )
        
        ; batt field
        (bufset-u8 tx-frame 7 (*(get-batt) 100))

        ; light field
        (if (= off 0)
            (bufset-u8 tx-frame 8 light)
            (bufset-u8 tx-frame 8 0)
        )
        
        ; beep field
        (if (= lock 1)
            (if (> (* (get-speed) 3.6) min-speed)
                (bufset-u8 tx-frame 9 1) ; beep lock
                (bufset-u8 tx-frame 9 0))
            (if (> feedback 0)
                (progn
                    (bufset-u8 tx-frame 9 1)
                    (setvar 'feedback (- feedback 1))
                )
                (bufset-u8 tx-frame 9 0)
            )
        )

        ; speed field
        (if (= show-batt-in-idle 1)
            (if (> (* (get-speed) 3.6) 1)
                (bufset-u8 tx-frame 10 (* (get-speed) 3.6))
                (bufset-u8 tx-frame 10 (*(get-batt) 100)))
            (bufset-u8 tx-frame 10 (* (get-speed) 3.6))
        )
        
        ; error field
        (if (= show-faults 1)
            (bufset-u8 tx-frame 11 (get-fault))
        )

        ; calc crc

        (setvar 'crc 0)
        (looprange i 2 12
            (setvar 'crc (+ crc (bufget-u8 tx-frame i))))
        (setvar 'c-out (bitwise-xor crc 0xFFFF)) 
        (bufset-u8 tx-frame 12 c-out)
        (bufset-u8 tx-frame 13 (shr c-out 8))

        ; write
        (uart-write tx-frame)
    )
)

(defun read-thd()
    (loopwhile t
        (progn
            (uart-read-bytes uart-buf 3 0)
            (if (= (bufget-u16 uart-buf 0) 0x55aa)
                (progn
                    (setvar 'len (bufget-u8 uart-buf 2))
                    (setvar 'crc len)
                    (if (> len 0) 
                        (progn
                            (uart-read-bytes uart-buf (+ len 4) 0)
                            (looprange i 0 len
                                (setvar 'crc (+ crc (bufget-u8 uart-buf i))))
                            (if (=(+(shl(bufget-u8 uart-buf (+ len 2))8) (bufget-u8 uart-buf (+ len 1))) (bitwise-xor crc 0xFFFF))
                                (progn
                                    (setvar 'code (bufget-u8 uart-buf 1))
                                    
                                    (if(= code 0x65)
                                        (inp uart-buf)
                                    )
                                    ;(if(= code 0x64)
                                        (outp uart-buf)
                                    ;)
                                )
                            )
                        )
                    )
                )
            )
        )
    )
)

; Spawn UART reading thread
(spawn 150 read-thd) 

(loopwhile t
    (progn
        ; If you do not have any R470 resistors or it still occours to press buttons randomly, try uncommenting this:
        ;(if (<= (* (get-speed) 3.6) min-speed)
        ;(progn
            (if (> buttonold (gpio-read 'pin-rx))
                (progn
                    (setvar 'presses (+ presses 1))
                    (setvar 'presstime (systime))
                )
                (if (> (- (systime) presstime) 2500) ;
                    (if (= (gpio-read 'pin-rx) 0) ; check if button is still pressed
                        (if (> (- (systime) presstime) 6000) ; instead check for long press
                            (progn
                                (if (= lock 0) ; it is locked? do not turn off
                                    (progn
                                        (setvar 'off 1) ; turn off
                                        (setvar 'feedback 1) ; beep feedback
                                    )
                                )
                                (setvar 'presstime (systime)) ; reset press time again
                                (setvar 'presses 0)
                            )
                        )
                        (progn ; else if button not pressed
                            (if (= presses 1) ; single press
                                (if (= off 1) ; is it off? turn on scooter again
                                    (progn
                                        (setvar 'off 0) ; turn on
                                        (setvar 'feedback 1) ; beep feedback
                                    )
                                    (setvar 'light (bitwise-xor light 1)) ; toggle light
                                )
                            )
                            
                            (if (>= presses 2) ; double press
                                (progn
                                    (if (> brake 0.4)
                                        (setvar 'lock (bitwise-xor lock 1))
                                        (progn
                                            (if (= speedmode 1) ; is drive?
                                                (progn 
                                                    (conf-set 'max-speed (/ speed-sport 3.6))
                                                    (conf-set 'l-watt-max watt-sport)
                                                    (setvar 'speedmode 4) ; to sport
                                                )
                                                (if (= speedmode 2) ; is eco?
                                                    (progn
                                                        (conf-set 'max-speed (/ speed-drive 3.6))
                                                        (conf-set 'l-watt-max watt-drive)
                                                        (setvar 'speedmode 1) ; to drive
                                                    )
                                                    (if (= speedmode 4) ; is sport?
                                                        (progn
                                                            (conf-set 'max-speed (/ speed-eco 3.6))
                                                            (conf-set 'l-watt-max watt-eco)
                                                            (setvar 'speedmode 2) ; to eco
                                                        )
                                                    )
                                                )
                                            )
                                        )
                                    )
                                )
                            )
                                
                            (setvar 'presses 0)
                        )
                    )
                )
            )

            (setvar 'buttonold (gpio-read 'pin-rx))
        ;))
        (sleep 0.01)
    )
)