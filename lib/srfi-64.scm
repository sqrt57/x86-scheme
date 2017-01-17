(define (test-begin . rest) (write (list "Starting test group" (car rest))))

(define (test-end . rest) (write (list "Finished test group" (car rest))))

(define (test-assert name condition)
  (if condition
      #t
      (write (list name "failed"))))

(define (test-eq name x y)
  (test-assert name (eq? x y)))

