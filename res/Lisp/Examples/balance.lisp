; Balance robot controller written in lisp

(defun #abs (x) (if (> x 0) x (- x)))

(defun #pos-x ()
    (* 0.5 (+
        (progn (select-motor 1) (get-dist))
        (progn (select-motor 2) (get-dist))
)))

(defun #set-output (left right)
    (progn
        (select-motor 1)
        (set-current-rel right)
        (select-motor 2)
        (set-current-rel left)
        (timeout-reset)
))

(defun #speed-x ()
    (* 0.5 (+
        (progn (select-motor 1) (get-speed))
        (progn (select-motor 2) (get-speed))
)))

(define #yaw-set (rad2deg (ix (get-imu-rpy) 2)))
(define #pos-set (#pos-x))

(define #pitch-set 0)
(define #was-running 0)

(define #kp 0.014)
(define #kd 0.0016)

(define #p-kp 50.0)
(define #p-kd -33.0)

(define #y-kp 0.003)
(define #y-kd 0.0003)

(define #enable-pos 1)
(define #enable-yaw 1)

; This is received from the QML-program which acts as a remote control for the robot
(defun proc-data (data)
    (progn
        (define #enable-pos (bufget-u8 data 4))
        (define #enable-yaw (bufget-u8 data 5))
        
        (if (= #enable-pos 1)
            (progn
                (define #pos-set (+ #pos-set (* (bufget-u8 data 0) 0.002)))
                (define #pos-set (- #pos-set (* (bufget-u8 data 1) 0.002)))
        ) nil)
        
        (if (= #enable-yaw 1)
            (progn
                (define #yaw-set (- #yaw-set (* (bufget-u8 data 2) 0.5)))
                (define #yaw-set (+ #yaw-set (* (bufget-u8 data 3) 0.5)))
        ) nil)
        
        (if (> #yaw-set 360) (define #yaw-set (- #yaw-set 360)) nil)
        (if (< #yaw-set 0) (define #yaw-set (+ #yaw-set 360)) nil)
))

(defun event-handler ()
    (progn
        (recv ((event-data-rx . (? data)) (proc-data data))
              (_ nil))
        (event-handler)
))

(event-register-handler (spawn event-handler))
(event-enable 'event-data-rx)

(define #t-last (systime))
(define #it-rate 0)
(define #it-rate-filter 0)
(defun #filter (val sample)
    (- val (* 0.01 (- val sample)))
)

; Sleep after boot to wait for IMU to settle
(if (< (secs-since 0) 5) (sleep 5) nil)

(loopwhile t
    (progn
        (define #pitch (rad2deg (ix (get-imu-rpy) 1)))
        (define #yaw (rad2deg (ix (get-imu-rpy) 2)))
        (define #pitch-rate (ix (get-imu-gyro) 1))
        (define #yaw-rate (ix (get-imu-gyro) 2))
        (define #pos (+ (#pos-x) (* #pitch 0.00122))) ; Includes pitch compensation
        (define #speed (#speed-x))

        ; Loop rate measurement
        (define #it-rate (/ 1.0 (secs-since #t-last)))
        (define #t-last (systime))
        (define #it-rate-filter (#filter #it-rate-filter #it-rate))
                
        (if (< (#abs #pitch) (if (= #was-running 1) 45 10))
            (progn
                (define #was-running 1)
                
                (if (= #enable-pos 0) (define #pos-set #pos) nil)
                (if (= #enable-yaw 0) (define #yaw-set #yaw) nil)
                
                (define #pos-err (- #pos-set #pos))
                (define #pitch-set (+ (* #pos-err #p-kp) (* #speed #p-kd)))
                
                (define #yaw-err (- #yaw-set #yaw))
                (if (> #yaw-err 180) (define #yaw-err (- #yaw-err 360)) nil)
                (if (< #yaw-err -180) (define #yaw-err (+ #yaw-err 360)) nil)
                
                (define #yaw-out (+ (* #yaw-err #y-kp) (* #yaw-rate #y-kd)))
                (define #ctrl-out (+ (* #kp (- #pitch #pitch-set)) (* #kd #pitch-rate)))
                
                (#set-output (+ #ctrl-out #yaw-out) (- #ctrl-out #yaw-out))
            )
            
            (progn
                (define #was-running 0)
                (#set-output 0 0)
                (define #pos-set #pos)
                (define #yaw-set #yaw)
            )
        )
        
        (yield 1) ; Run as fast as possible
))
