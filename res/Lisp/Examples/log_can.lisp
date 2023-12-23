; Can ID of VESC express
(def esp-can-id 2)

; Log rate in Hz
(def rate-hz 20.0)

; Log GNSS. Setting this to 1 will enable GNSS-logging. Note that the log won't be started
; until a valid position is available if GNSS-logging is enabled.
(def log-gnss false)

; Close log if voltage goes below this value
(def vin-min 9.0)

; List with log fields and values. Format:
;
; (optKey optName optUnit optPrecision optIsRel optIsTime value-function)
;
; All entries except value-function are optional and
; default values will be used if they are left out.
(def loglist '(
        ("v_in" "V" "Input Voltage"     (get-vin))
        ("roll"                         (ix (get-imu-rpy) 0))
        ("pitch"                        (ix (get-imu-rpy) 1))
        ("yaw"                          (ix (get-imu-rpy) 2))
        ("kmh_vesc" "km/h" "Speed VESC" (* (get-speed) 3.6))
))

; The code below runs the logging and can be left as is

(looprange row 0 (length loglist)
    (let (
            (field (ix loglist row))
            (get-field
                (fn (type default)
                    (let ((f (first field)))
                        (if (eq (type-of f) type)
                            (progn
                                (setvar 'field (rest field))
                                f
                            )
                            default
            ))))
            (key       (get-field type-array (str-from-n row "Field %d")))
            (unit      (get-field type-array ""))
            (name      (get-field type-array key))
            (precision (get-field type-i 2))
            (is-rel    (get-field type-symbol false))
            (is-time   (get-field type-symbol false))
        )
        (log-config-field
            esp-can-id ; CAN id
            row ; Field
            key ; Key
            name ; Name
            unit ; Unit
            precision ; Precision
            is-rel ; Is relative
            is-time ; Is timestamp
        )
))

(log-start
    esp-can-id ; CAN id
    (length loglist) ; Field num
    rate-hz ; Rate Hz
    true ; Append time
    log-gnss ; Append gnss
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

(defun voltage-monitor () {
        (if (< (get-vin) vin-min)
            (log-stop esp-can-id)
        )
        (sleep 0.01)
})

(spawn 30 voltage-monitor)

(loopwhile t {
        (log-send-f32 esp-can-id 0
            (map
                (fn (x) (eval (ix x -1)))
                loglist
            )
        )
        (sleep (/ 1.0 rate-hz))
})
