;m365 dashboard free for all by Netzpfuscher
;red=5V black=GND yellow=COM-TX (UART-HDX) green=COM-RX (button)

;****User parameters****
;Calibrate throttle min max
(define cal-thr-lo 32.0)
(define cal-thr-hi 178.0)

;Calibrate brake min max
(define cal-brk-lo 32.0)
(define cal-brk-hi 178.0)

(define light-default 0)
(define show-faults 1)

;****Code section****
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

(defun inp (buffer) ;Frame 0x65
    (progn
    (setvar 'throttle (/(-(bufget-u8 uart-buf 4) cal-thr-lo) cal-thr-hi))
    (setvar 'brake (/(-(bufget-u8 uart-buf 5) cal-brk-lo) cal-brk-hi))
    (if (> brake 0.01)
        (set-brake-rel brake)
        (set-current-rel throttle)
    )
))

(defun outp (buffer) ;Frame 0x64
    (progn
    (setvar 'crc 0)
    (looprange i 2 12
        (setvar 'crc (+ crc (bufget-u8 tx-frame i))))
    (setvar 'c-out (bitwise-xor crc 0xFFFF)) 
    (bufset-u8 tx-frame 12 c-out)
    (bufset-u8 tx-frame 13 (shr c-out 8))
    (uart-write tx-frame)
))

(defun read-thd ()
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
                                    (if(= (bufget-u8 uart-buf 1) 0x65)
                                        (inp uart-buf))
                                    (if(= (bufget-u8 uart-buf 1) 0x64)
                                        (outp uart-buf))
                                    )
))))))))

(spawn 150 read-thd) ; Run UART in its own thread

(loopwhile t
    (progn
        (if (> buttonold (gpio-read 'pin-rx))
            (setvar 'light (bitwise-xor light 1))
        )
        (setvar 'buttonold (gpio-read 'pin-rx))
        (bufset-u8 tx-frame 7 (*(get-batt) 100))
        (bufset-u8 tx-frame 8 light)
        (bufset-u8 tx-frame 10 (* (get-speed) 3.6))
        (if (= show-faults 1)
            (bufset-u8 tx-frame 11 (get-fault))
        )
        (sleep 0.1)
))
