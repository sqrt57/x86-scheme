(define test-begin (lambda rest (write (list "Starting test group" (car rest)))))
(define test-end (lambda rest (write (list "Finished test group" (car rest)))))

(define test-assert (lambda (name condition)
                      (if condition
                          #t
                          (write (list name "failed")))))
(define test-eq (lambda (name x y)
                  (test-assert name (eq? x y))))
