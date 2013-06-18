(defun start-walk
  (&key (real t) (vel-mode nil) (before-get-in-vehicle nil))

  (let (prev-av lleg-av rleg-av
		prev-leg-width dest-leg-width (thre-leg-width 30) (set-high-gain-p nil))
    (when real
      (real2model)
      (model2real :wait t :time 10)
      (atlas-set-servo-gain-by-torque-limit 1.0 :legs)
      (atlas-set-servo-gain-by-torque-limit 2.0 :torso)
      (atlas-set-servo-gain-by-torque-limit 4.0 :arms)
      (atlas-set-servo-gain-by-torque-limit 5.0 :head)
      (set-user))

    (setq prev-av (send *atlas* :angle-vector))
    (fix-leg)
    (setq prev-leg-width (abs (elt (apply #'v- (send-all (send *atlas* :legs :end-coords) :worldpos)) 1)))

    (when before-get-in-vehicle
      (send *atlas* :angle-vector
	    #f(0.0 0.0 0.0 23.5 114.5 0.0 -75.0 17.0 0.0 -135.0 129.6 -71.6129 79.9998 60.7501 0.0 61.5619 -1.79235 -35.568 1.79235 -25.994 0.012938 61.5446 1.91967 -35.5619 -1.91967 -25.9828 0.01294 0.0))
      (grasp :arm :rarm :real real)
      (ungrasp :arm :larm :real real)
      )

    ;; 足だけwalk-poseにする
    (send *atlas* :walk-pose)
    (setq dest-leg-width (abs (elt (apply #'v- (send-all (send *atlas* :legs :end-coords) :worldpos)) 1)))

    (when (> (abs (- prev-leg-width dest-leg-width)) thre-leg-width)
      (setq set-high-gain-p t))

    (setq lleg-av (send *atlas* :lleg :angle-vector))
    (setq rleg-av (send *atlas* :rleg :angle-vector))
    (send *atlas* :angle-vector prev-av)
    (send *atlas* :lleg :angle-vector lleg-av)
    (send *atlas* :rleg :angle-vector rleg-av)
    (send *atlas* :head :neck-p :joint-angle 50)
    (fix-leg)
    (cond (before-get-in-vehicle
	   (send *atlas* :move-centroid-on-foot :both '(:rleg :lleg)
		 :target-centroid-pos (v+ (float-vector 50 0 0) (apply #'midpoint 0.5 (send-all (send *atlas* :legs :end-coords :parent) :worldpos)))))
	  (t (move-centroid)))

    (when real
      (atlas-set-servo-gain-by-torque-limit 0.01 :legs)
      (unix::usleep (* 5 1000 1000))
      (model2real :wait t :time 10000)
      (atlas-set-servo-gain-by-torque-limit 1.0 :legs)
      (unix::usleep (* 5 1000 1000))
      (send *ri* :start-st)
      (when vel-mode
	(warning-message 1 " rosrun pr2_teleop teleop_pr2_keyboard~%")
	(send *ri* :cmd-vel-mode))
      ))
  t)

(defun finish-walk
  (&key (real t) (mode :user))

  (when real
    (when (or (equal mode :stand) (equal mode :manip) (equal mode :my-manip))
      (modify-leg-stride #f(0 50 0)))
    (send *ri* :stop-auto-balancer)
    (send *ri* :stop-st))
  (when (or (equal mode :stand) (equal mode :manip) (equal mode :my-manip))
    ;; 足だけbdi-stand-poseにする
    (let ((prev-av (send *atlas* :angle-vector))
          lleg-av rleg-av)
      (send *atlas* :bdi-stand-pose)
      (setq lleg-av (send *atlas* :lleg :angle-vector))
      (setq rleg-av (send *atlas* :rleg :angle-vector))
      (send *atlas* :angle-vector prev-av)
      (send *atlas* :lleg :angle-vector lleg-av)
      (send *atlas* :rleg :angle-vector rleg-av)
      (fix-leg)
      (move-centroid))
    (when real (model2real :wait t :time 5000)))

  (when real
    (case mode
      (:stand (set-stand))
      (:manip (set-manip))
      (:my-manip (manip))))
  t)

(defun walk-clickpos
  ()

  (let* ((robot-coords (send (send *atlas* :foot-midcoords) :copy-worldcoords))
         (clicked-coords (get-point))
         (walk-pos
          (when clicked-coords
            (scale 0.001 (send (send robot-coords
                                     :transformation clicked-coords) :worldpos)))))
    (when walk-pos
      (warn "[walk-clickpos] x: ~a~%  OK?  " (elt walk-pos 0))
      (unless (y-or-n-p) (return-from walk-clickpos))
      (send *ri* :go-pos (elt walk-pos 0) 0 0))))

(defun turn-clickpos
  ()

  (let* ((robot-coords (send (send *atlas* :foot-midcoords) :copy-worldcoords))
         (clicked-coords (get-point))
         (walk-pos
          (when clicked-coords
            (scale 0.001 (send (send robot-coords
                                     :transformation clicked-coords) :worldpos))))
         (turn-theta (rad2deg (atan2 (elt walk-pos 1) (elt walk-pos 0)))))
    (when walk-pos
      (warn "[turn-clickpos] theta: ~a~%  OK?  " turn-theta)
      (unless (y-or-n-p) (return-from turn-clickpos))
      (send *ri* :go-pos 0 0 turn-theta))))

(defun walk
  (x)

  ;; (warn "[walk] x: ~a~%  OK?  " x)
  ;; (unless (y-or-n-p) (return-from walk))
  (send *ri* :go-pos x 0 0))

(defun turn
  (theta &optional (lr nil))

  (setq theta (cond ((equal lr :left) (abs theta))
                    ((equal lr :right) (- (abs theta))(abs theta))
                    (t theta)))
  ;; (warn "[walk] theta: ~a~%  OK?  " theta)
  ;; (unless (y-or-n-p) (return-from turn))
  (send *ri* :go-pos 0 0 theta))

(defun move-neck-p
  (&optional (rel-angle 10) &key (zero nil))

  (cond 
   (zero (send *atlas* :head :neck-p :joint-angle 50))
   (t (send *atlas* :head :neck-p :joint-angle rel-angle :relative t)))  
  (fix-leg)
  (move-centroid)
  (model2real :wait t))

(defun move-waist-y
  (&optional (rel-angle 10) &key (zero nil))

  (cond 
   (zero (send *atlas* :torso :waist-y :joint-angle 0))
   (t (send *atlas* :torso :waist-y :joint-angle rel-angle :relative t)))
  (fix-leg)
  (move-centroid)
  (model2real :wait t))
