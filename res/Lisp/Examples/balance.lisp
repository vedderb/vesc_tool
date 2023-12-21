; Balance robot controller written in lisp

(defun #abs (x) (if (> x 0) x (- x)))

(defun #pos-x ()
    (* 0.5 (+
            {(select-motor 1) (get-dist)}
            {(select-motor 2) (get-dist)}
)))

(defun #set-output (left right) {
        (select-motor 1)
        (set-current-rel right)
        (select-motor 2)
        (set-current-rel left)
        (timeout-reset)
})

(defun #speed-x ()
    (* 0.5 (+
            {(select-motor 1) (get-speed)}
            {(select-motor 2) (get-speed)}
)))

(def #yaw-set (rad2deg (ix (get-imu-rpy) 2)))
(def #pos-set (#pos-x))

(def #pitch-set 0)
(def #was-running 0)

(def #kp 0.014)
(def #kd 0.0016)

(def #p-kp 50.0)
(def #p-kd -33.0)

(def #y-kp 0.003)
(def #y-kd 0.0003)

(def #enable-pos 1)
(def #enable-yaw 1)

; This is received from the QML-program which acts as a remote control for the robot
(defun proc-data (data) {
        (setq #enable-pos (bufget-u8 data 4))
        (setq #enable-yaw (bufget-u8 data 5))
        
        (if (= #enable-pos 1) {
                (def #pos-set (+ #pos-set (* (bufget-u8 data 0) 0.002)))
                (def #pos-set (- #pos-set (* (bufget-u8 data 1) 0.002)))
        })
        
        (if (= #enable-yaw 1) {
                (def #yaw-set (- #yaw-set (* (bufget-u8 data 2) 0.5)))
                (def #yaw-set (+ #yaw-set (* (bufget-u8 data 3) 0.5)))
        })
        
        (if (> #yaw-set 360) (def #yaw-set (- #yaw-set 360)) nil)
        (if (< #yaw-set 0) (def #yaw-set (+ #yaw-set 360)) nil)
})

(defun event-handler ()
    (loopwhile t
        (recv
            ((event-data-rx . (? data)) (proc-data data))
            (_ nil)
)))

(event-register-handler (spawn event-handler))
(event-enable 'event-data-rx)

(def #t-last (systime))
(def #it-rate 0)
(def #it-rate-filter 0)
(defun #filter (val sample)
    (- val (* 0.01 (- val sample)))
)

; Sleep after boot to wait for IMU to settle
(if (< (secs-since 0) 5) (sleep 5) nil)

(loopwhile t {
        (def #pitch (rad2deg (ix (get-imu-rpy) 1)))
        (def #yaw (rad2deg (ix (get-imu-rpy) 2)))
        (def #pitch-rate (ix (get-imu-gyro) 1))
        (def #yaw-rate (ix (get-imu-gyro) 2))
        (def #pos (+ (#pos-x) (* #pitch 0.00122))) ; Includes pitch compensation
        (def #speed (#speed-x))
        
        ; Loop rate measurement
        (def #it-rate (/ 1.0 (secs-since #t-last)))
        (def #t-last (systime))
        (def #it-rate-filter (#filter #it-rate-filter #it-rate))
        
        (if (< (#abs #pitch) (if (= #was-running 1) 45 10))
            {
                (def #was-running 1)
                
                (if (= #enable-pos 0) (def #pos-set #pos) nil)
                (if (= #enable-yaw 0) (def #yaw-set #yaw) nil)
                
                (def #pos-err (- #pos-set #pos))
                (def #pitch-set (+ (* #pos-err #p-kp) (* #speed #p-kd)))
                
                (def #yaw-err (- #yaw-set #yaw))
                (if (> #yaw-err 180) (def #yaw-err (- #yaw-err 360)) nil)
                (if (< #yaw-err -180) (def #yaw-err (+ #yaw-err 360)) nil)
                
                (def #yaw-out (+ (* #yaw-err #y-kp) (* #yaw-rate #y-kd)))
                (def #ctrl-out (+ (* #kp (- #pitch #pitch-set)) (* #kd #pitch-rate)))
                
                (#set-output (+ #ctrl-out #yaw-out) (- #ctrl-out #yaw-out))
            }
            { ; else
                (def #was-running 0)
                (#set-output 0 0)
                (def #pos-set #pos)
                (def #yaw-set #yaw)
            }
        )
        
        (yield 1) ; Run as fast as possible
})
