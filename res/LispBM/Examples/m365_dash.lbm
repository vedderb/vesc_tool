;m365 dashboard free for all by Netzpfuscher
;red=5V black=GND yellow=COM-TX (UART-HDX) green=COM-RX (button)

;****User parameters****
;Calibrate throttle min max
(def cal-thr-lo 32.0)
(def cal-thr-hi 178.0)

;Calibrate brake min max
(def cal-brk-lo 32.0)
(def cal-brk-hi 178.0)

(def light-default 0)
(def show-faults 1)

;****Code section****
(uart-start 115200 'half-duplex)
(gpio-configure 'pin-rx 'pin-mode-in-pu)

(def tx-frame (array-create 14))
(bufset-u16 tx-frame 0 0x55AA)
(bufset-u16 tx-frame 2 0x0821)
(bufset-u16 tx-frame 4 0x6400)

(def uart-buf (array-create 64))
(def throttle 0)
(def brake 0)
(def buttonold 0)
(def light 0)
(setq light light-default)
(def c-out 0)

(defun inp (buffer) { ;Frame 0x65
        (setq throttle (/(-(bufget-u8 uart-buf 4) cal-thr-lo) cal-thr-hi))
        (setq brake (/(-(bufget-u8 uart-buf 5) cal-brk-lo) cal-brk-hi))
        (if (> brake 0.01)
            (set-brake-rel brake)
            (set-current-rel throttle)
        )
})

(defun outp (buffer) { ;Frame 0x64
        (setq crc 0)
        (looprange i 2 12
            (setq crc (+ crc (bufget-u8 tx-frame i)))
        )
        (setq c-out (bitwise-xor crc 0xFFFF))
        (bufset-u8 tx-frame 12 c-out)
        (bufset-u8 tx-frame 13 (shr c-out 8))
        (uart-write tx-frame)
})

(defun read-thd ()
    (loopwhile t {
            (uart-read-bytes uart-buf 3 0)
            (if (= (bufget-u16 uart-buf 0) 0x55aa) {
                    (setq len (bufget-u8 uart-buf 2))
                    (setq crc len)
                    (if (> len 0) {
                            (uart-read-bytes uart-buf (+ len 4) 0)
                            (looprange i 0 len
                            (setq crc (+ crc (bufget-u8 uart-buf i))))
                            (if (=(+(shl(bufget-u8 uart-buf (+ len 2))8) (bufget-u8 uart-buf (+ len 1))) (bitwise-xor crc 0xFFFF)) {
                                    (if(= (bufget-u8 uart-buf 1) 0x65)
                                    (inp uart-buf))
                                    (if(= (bufget-u8 uart-buf 1) 0x64)
                                    (outp uart-buf))
                            })
                    })
            })
    })
)

(spawn 150 read-thd) ; Run UART in its own thread

(loopwhile t {
        (if (> buttonold (gpio-read 'pin-rx))
            (setq light (bitwise-xor light 1))
        )
        (setq buttonold (gpio-read 'pin-rx))
        (bufset-u8 tx-frame 7 (*(get-batt) 100))
        (bufset-u8 tx-frame 8 light)
        (bufset-u8 tx-frame 10 (* (get-speed) 3.6))
        (if (= show-faults 1)
            (bufset-u8 tx-frame 11 (get-fault))
        )
        (sleep 0.1)
})
