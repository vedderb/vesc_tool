; This example comminucates with a Grayhill 3J Gen 2.0 Joystick. It
; uses the J1939 CAN-standard.

(define js-sa 0xF1) ; Grayhill joystick source address
(define my-id 2)

; ID to send data to the joysick
(define id-send (bits-enc-int (bits-enc-int 0x18EF0000u32 8 js-sa 8) 0 my-id 8))

; Received X- and Y-axis values from the joystick
(define x-axis 0)
(define y-axis 0)

; Counter for received CAN-frames
(define can-cnt 0)

; Handler for EID CAN-frames
(define proc-eid (lambda (id data)
    (if (= id 0x18ff03f1u32)
        (progn
            (define can-cnt (+ can-cnt 1))
            
            ; Note: the axes are 10 bits in the datasheet, but they change in
            ; steps of 2 percent, so the lower bits are always 0. Not sure
            ; why they don't use just 7 bits for that.
            
            (define x-axis (* 0.001
                (* (if (= (bits-dec-int (bufget-u8 data 0) 2 1) 0) 1.0 -1.0)
                (+ (bits-dec-int (bufget-u8 data 0) 6 2) (* 4 (bufget-u8 data 1))
            ))))
            
            (define y-axis (* 0.001
                (* (if (= (bits-dec-int (bufget-u8 data 2) 2 1) 0) 1.0 -1.0)
                (+ (bits-dec-int (bufget-u8 data 2) 6 2) (* 4 (bufget-u8 data 3))
            ))))
                        
            ; Decode the buttons that are currently pressed
            (define btn-lst (list
                (bits-dec-int (bufget-u8 data 5) 6 2)
                (bits-dec-int (bufget-u8 data 5) 4 2)
                (bits-dec-int (bufget-u8 data 5) 2 2)
                (bits-dec-int (bufget-u8 data 5) 0 2)
                (bits-dec-int (bufget-u8 data 6) 6 2)
                (bits-dec-int (bufget-u8 data 6) 4 2)
                (bits-dec-int (bufget-u8 data 6) 2 2)
            ))
            
            ; Save memory by freeing data when done. This can be omitted as GC
            ; will free it in the next run, but doing it prevents the memory
            ; usage from increasing more than needed.
            (free data)
            
;            (set-duty y-axis)
;            (timeout-reset)
        )
        nil
)))

; This function waits for events from the C code and calls the
; handlers for them when the events arrive.
(defun event-handler ()
    (loopwhile t
        (recv ((event-can-eid (? id) . (? data)) (proc-eid id data))
              (_ nil))
))

; Set LEDs in button.
; 0: off, 1: on, 2: slow blink, 3: med blink, 4: fast blink
; btn: Button number
; l: Left LED
; c: Center LED
; r: Right LED
(define set-btn-leds (lambda (btn l c r)
    (progn
        (define msg (list btn (bits-enc-int l 4 c 4) r))
        (can-send-eid id-send msg)
)))

; Spawn the event handler thread and pass the ID it returns to C
(event-register-handler (spawn 150 event-handler))

; Enable the CAN event for extended ID (EID) frames
(event-enable 'event-can-eid)

(define btn-now 1)

(loopwhile t
    (progn 
        (set-btn-leds btn-now 1 4 1)
        (yield 1500000)
        (set-btn-leds btn-now 0 0 0)
        
        (define btn-now (+ btn-now 1))
        (if (= btn-now 6) (define btn-now 7)) ; Index 6 is missing, so skip it
        (if (> btn-now 7) (define btn-now 1))
))
