; Balance robot controller written in lisp

; This is received from the QML-program which acts as a remote control for the robot
(define proc-data (lambda (data)
    (progn
        (define pos-set (+ pos-set (* (ix 0 data) 0.002)))
        (define pos-set (- pos-set (* (ix 1 data) 0.002)))
        (define yaw-set (- yaw-set (* (ix 2 data) 0.5)))
        (define yaw-set (+ yaw-set (* (ix 3 data) 0.5)))
        
        (if (> yaw-set 360) (define yaw-set (- yaw-set 360)) nil)
        (if (< yaw-set 0) (define yaw-set (+ yaw-set 360)) nil)
)))

(define event-handler (lambda ()
    (progn
        (recv ((signal-data-rx . (? data)) (proc-data data))
              (_ nil))
        (event-handler)
)))

(event-register-handler (spawn '(event-handler)))
(event-enable "event-data-rx")

(define abs (lambda (x) (if (> x 0) x (- x))))

(define set-output (lambda (left right)
    (progn
        (select-motor 1)
        (set-current-rel right)
        (select-motor 2)
        (set-current-rel left)
        (timeout-reset)
)))

(define pos-x (lambda ()
    (progn
        (select-motor 1)
        (define m1-p (get-dist))
        (select-motor 2)
        (define m2-p (get-dist))
        (* 0.5 (+ m1-p m2-p))
)))

(define speed-x (lambda ()
    (progn
        (select-motor 1)
        (define m1-s (get-speed))
        (select-motor 2)
        (define m2-s (get-speed))
        (* 0.5 (+ m1-s m2-s))
)))

(define pitch-set 0)
(define yaw-set (* (ix 2 (get-imu-rpy)) 57.29577951308232))
(define pos-set (pos-x))

(define was-running 0)
(define t-last (systime))
(define pos-last (pos-x))

(define kp 0.016)
(define kd 0.0018)

(define p-kp 50.0)
(define p-kd -33.0)

(define y-kp 0.003)
(define y-kd 0.0003)

(define f (lambda ()
    (progn
        (define pitch (* (ix 1 (get-imu-rpy)) 57.29577951308232))
        (define yaw (* (ix 2 (get-imu-rpy)) 57.29577951308232))
        (define pitch-rate (ix 1 (get-imu-gyro)))
        (define yaw-rate (ix 2 (get-imu-gyro)))
        (define pos (+ (pos-x) (* pitch 0.00122))) ; Includes pitch compensation
        (define speed (speed-x))

        ; Loop rate measurement
        (define it-rate (/ 1 (secs-since t-last)))
        (define t-last (systime))
        
        (if (< (abs pitch) (if (= was-running 1) 45 10))
            (progn
                (define was-running 1)
                
                ; Uncomment these to run the pitch controller by itself
;                (define pos-set pos)
;                (define yaw-set yaw)
                
                (define pos-err (- pos-set pos))
                (define pitch-set (+ (* pos-err p-kp) (* speed p-kd)))
                
                (define yaw-err (- yaw-set yaw))
                (if (> yaw-err 180) (define yaw-err (- yaw-err 360)) nil)
                (if (< yaw-err -180) (define yaw-err (+ yaw-err 360)) nil)
                
                (define yaw-out (+ (* yaw-err y-kp) (* yaw-rate y-kd)))
                (define ctrl-out (+ (* kp (- pitch pitch-set)) (* kd pitch-rate)))
                
                (set-output (+ ctrl-out yaw-out) (- ctrl-out yaw-out))
            )
            
            (progn
                (define was-running 0)
                (set-output 0 0)
                (define pos-set pos)
                (define yaw-set yaw)
            )
        )
        
        (yield 1) ; Run as fast as possible
        (f)
)))

(if (< (systime) 50000) (yield 5000000) nil) ; Sleep after boot to wait for IMU to settle
(f)
