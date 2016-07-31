; Delimited continuation
(write
  (call/cc (lambda (ex)
             (+ ((lambda () 3))
                ((lambda () (ex 1) 4))))))

; Full continuation
(define cont 0)

(write (+ 1
          (call/cc (lambda (ex)
                     (set! cont ex)
                     5))))

(cont 10)
(cont 15)

