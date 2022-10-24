; Can ID of VESC express
(def esp-can-id 2)

; Log rate in Hz
(def rate-hz 20.0)

; Log GNSS. Setting this to 1 will enable GNSS-logging. Note that the log won't be started
; until a valid position is available if GNSS-logging is enabled.
(def log-gnss 0)

; Close log if voltage goes below this value
(def vin-min 9.0)

; List with log fields and values. Format:
;
; (key optName optUnit optPrecision value-function)
;
; optName, optUnit and optPrecision are optional and
; default values will be used if they are left out.
(def loglist '(
        ("v_in" "Input Voltage" "V"     (get-vin))
        ("roll"                         (ix (get-imu-rpy) 0))
        ("pitch"                        (ix (get-imu-rpy) 1))
        ("yaw"                          (ix (get-imu-rpy) 2))
        ("kmh_vesc" "Speed VESC" "km/h" (* (get-speed) 3.6))
))

; The code below runs the logging and can be left as is

(looprange i 0 (length loglist)
    (let (
            (field (ix loglist i))
            (p (ix field -2))
            (has-prec (eq (type-of p) type-i))
            (prec (if has-prec p 2))
        )
        (match (if has-prec (- (length field) 1) (length field))
            (2
                (log-config-field
                    esp-can-id ; CAN id
                    i ; Field
                    (ix field 0) ; Key
                    (ix field 0) ; Name
                    "" ; Unit
                    prec ; Precision
                    0 ; Is relative
                    0 ; Is timestamp
                )
            )
            
            (3
                (log-config-field
                    esp-can-id ; CAN id
                    i ; Field
                    (ix field 0) ; Key
                    (ix field 0) ; Name
                    (ix field 1) ; Unit
                    prec ; Precision
                    0 ; Is relative
                    0 ; Is timestamp
                )
            )
            
            (4
                (log-config-field
                    esp-can-id ; CAN id
                    i ; Field
                    (ix field 0) ; Key
                    (ix field 1) ; Name
                    (ix field 2) ; Unit
                    prec ; Precision
                    0 ; Is relative
                    0 ; Is timestamp
                )
            )
            
            (_ (print (str-from-n i "Row %d is invalid")))
        )
    )
)

(log-start
    esp-can-id ; CAN id
    (length loglist) ; Field num
    rate-hz ; Rate Hz
    1 ; Append time
    log-gnss ; Append gnss
    log-gnss ; Append gnss time
)

; Close the log on the shutdown event
(defun event-handler ()
    (loopwhile t
        (recv
            (event-shutdown (log-stop esp-can-id))
            (_ nil) ; Ignore other events
)))

(event-register-handler (spawn 30 event-handler))
(event-enable 'event-shutdown)

(defun voltage-monitor ()
    (progn
        (if (< (get-vin) vin-min)
            (log-stop esp-can-id)
        )
        (sleep 0.01)
))

(spawn 30 voltage-monitor)

(loopwhile t
    (progn
        (log-send-f32 esp-can-id 0
            (map
                (fn (x) (eval (ix (ix loglist x) -1)))
                (range (length loglist))
            )
        )
        (sleep (/ 1.0 rate-hz))
))
