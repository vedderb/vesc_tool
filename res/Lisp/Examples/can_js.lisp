; This example comminucates with a Grayhill 3J Gen 2.0 Joystick. It
; uses the J1939 CAN-standard.

(define js-sa 0xF1) ; Grayhill joystick source address
(define my-id 2)

; ID to send data to the joysick
(define id-send (bits-enc-int (bits-enc-int 0x18EF0000 8 js-sa 8) 0 my-id 8))

; Received X- and Y-axis values from the joystick
(define x-axis 0)
(define y-axis 0)

; Counter for received CAN-frames
(define can-cnt 0)

; Handler for EID CAN-frames
(define proc-eid (lambda (id data)
    (if (num-eq id 0x18ff03f1)
        (progn
            (define can-cnt (+ can-cnt 1))
            
            ; Note: the axes are 10 bits in the datasheet, but they change in
            ; steps of 2 percent, so the lower bits are always 0. Not sure
            ; why they don't use just 7 bits for that.
            
            (define x-axis (* 0.001
                (* (if (= (bits-dec-int (ix 0 data) 2 1) 0) 1.0 -1.0)
                (+ (bits-dec-int (ix 0 data) 6 2) (* 4 (ix 1 data))
            ))))
            
            (define y-axis (* 0.001
                (* (if (= (bits-dec-int (ix 2 data) 2 1) 0) 1.0 -1.0)
                (+ (bits-dec-int (ix 2 data) 6 2) (* 4 (ix 3 data))
            ))))
                        
            ; Decode the buttons that are currently pressed
            (define btn-lst (list
                (bits-dec-int (ix 5 data) 6 2)
                (bits-dec-int (ix 5 data) 4 2)
                (bits-dec-int (ix 5 data) 2 2)
                (bits-dec-int (ix 5 data) 0 2)
                (bits-dec-int (ix 6 data) 6 2)
                (bits-dec-int (ix 6 data) 4 2)
                (bits-dec-int (ix 6 data) 2 2)
            ))
            
;            (set-duty y-axis)
;            (timeout-reset)
        )
        nil
)))

; This function waits for events from the C code and calls the
; handlers for them when the events arrive.
(define event-handler (lambda ()
    (progn
        (recv ((signal-can-eid (? id) . (? data)) (proc-eid id data))
              (_ nil))
        (event-handler) ; Call self again to make this a loop
)))

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

(define btn-now 1)

(define btn-fun (lambda ()
    (progn 
        (set-btn-leds btn-now 1 4 1)
        (yield 1500000)
        (set-btn-leds btn-now 0 0 0)
        
        (define btn-now (+ btn-now 1))
        (if (= btn-now 6) (define btn-now 7)) ; Index 6 is missing, so skip it
        (if (> btn-now 7) (define btn-now 1))
        
        (btn-fun) ; Call self again to make this a loop
)))

; Spawn the event handler thread and pass the ID it returns to C
(event-register-handler (spawn event-handler))

; Enable the CAN event for extended ID (EID) frames
(event-enable "event-can-eid")

; Start the button-blink-function from this thread
(btn-fun)
