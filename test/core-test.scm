; Core scheme functionality tests
;
; This file is part of x86-scheme project.
; Copyright (c) 2015-2017, Dmitry Grigoryev

(test-begin "core" 2)

(test-equal "+0" (+) 0)
(test-equal "+1" (+ 1) 1)
(test-equal "+2" (+ 1 2) 3)
(test-equal "+3" (+ 1 2 3) 6)

(test-equal "-0" (-) 0)
(test-equal "-1" (- 1) (- 0 1))
(test-equal "-2" (- 5 1) 4)
(test-equal "-3" (- 5 1 2) 2)

(define (tst)
  (define x 1)
  (begin
    (define y 7)
    (define x 3))
  (+ x y))
(test-assert "begin" (= (tst) 10))

(test-assert "eq" (eq? (quote a) (quote a)))

(test-eq "not-#t" (not #t) #f)
(test-eq "not-#f" (not #f) #t)
(test-eq "not-str" (not "abc") #f)

(test-assert "pair?-pair" (pair? (quote (a . b))))
(test-assert "pair?-empty-list" (not (pair? (quote ()))))
(test-assert "symbol?-#t" (symbol? (quote df)))
(test-assert "symbol?-#f" (not (symbol? "dgf")))
(test-assert "procedure?-#t" (procedure? (lambda () (quote w))))
(test-assert "procedure?-#f" (not (procedure? (quote (lambda () (quote w))))))
(test-assert "procedure?-native" (procedure? car))
(test-assert "procedure?-continuation"
             (procedure? (call/cc (lambda (x) (x x)))))
(test-assert "number?-#t" (number? 25))
(test-assert "number?-#f" (not (number? (quote t))))
(test-assert "string?-#t" (string? "Hello world"))
(test-assert "string?-#f" (not (string? (quote t))))
(test-assert "boolean?-1" (boolean? #t))
(test-assert "boolean?-2" (boolean? #f))
(test-assert "boolean?-#f" (not (boolean? (quote #f))))

(test-assert "null?-empty-list" (null? (quote ())))
(test-assert "null?-symbol" (not (null? (quote a))))

(test-assert "or2-#t-#f" (or2 #t #f))
(test-assert "or2-#f-#t" (or2 #f #t))
(test-assert "or2-#t-#t" (or2 #t #t))
(test-assert "or2-#f-#f" (not (or2 #f #f)))

(test-assert "and2-#t-#f" (not (and2 #t #f)))
(test-assert "and2-#f-#t" (not (and2 #f #t)))
(test-assert "and2-#t-#t" (and2 #t #t))
(test-assert "and2-#f-#f" (not (and2 #f #f)))

(test-assert "string=?-#t" (string=? "abc" "abc"))
(test-assert "string=?-#f-1" (not (string=? "abc" "def")))
(test-assert "string=?-#f-2" (not (string=? "abc" "abcd")))

(test-eq "string->symbol" (quote abba) (string->symbol "abba"))
(test-eq "string->symbol" (quote cons) (string->symbol "cons"))
(test-assert "symbol->string" (string=? "abkm" (symbol->string (quote abkm))))

(test-assert "string-append-2" (string=? "Hello world"
                                         (string-append-2 "Hello " "world")))
(test-assert "string-append"
             (string=? "Hello, world"
                       (string-append "Hello" ", " "world")))
(test-assert "string-append-0" (string=? "" (string-append)))

(test-assert "memq-#t" (memq (quote a) (quote (c b a x))))
(test-assert "memq-#f" (not (memq (quote d) (quote (c b a x)))))

(test-assert "has-feature?-#t" (has-feature? (quote x86-scheme)))
(test-assert "has-feature?-#f" (not (has-feature? (quote test-feature))))
(add-feature! (quote test-feature))
(test-assert "add-feature!" (has-feature? (quote test-feature)))

(test-equal "assq-#t"
            (assq (quote h) (quote ((a b) (h i) (e f))))
            (quote (h i)))
(test-assert "assq-#f" (not (assq (quote h) (quote ((a b) (c d))))))

(test-equal "map" (map (lambda (x) (+ 10 x))
                       (quote (1 2)))
                  (quote (11 12)))
(define l (quote ((a (b)) (c d))))
(define lc (alist-copy l))
(test-assert "alist-copy-new1" (not (eq? l lc)))
(test-assert "alist-copy-new2" (not (eq? (car l) (car lc))))
(test-equal "alist-copy" l lc)

(define pair (quote (a . b)))
(set-car! pair (quote c))
(test-equal "set-car!" pair (quote (c . b)))
(define pair (quote (a . b)))
(set-cdr! pair (quote d))
(test-equal "set-cdr!" pair (quote (a . d)))

(define env (create-environment))
(test-eq "new-env-parent" (environment-parent env) #f)
(test-eq "new-env-bindings" (environment-bindings env) (quote ()))
(set-environment-parent! env (global-environment))
(test-eq "set-env-parent" (environment-parent env) (global-environment))
(set-environment-bindings! env (quote ((a 1) (b 2))))
(test-equal "set-env-bindings" (environment-bindings env) (quote ((a 1) (b 2))))

(test-equal "eval"
            (eval-current-environment
              (quote (car (quote (a . b)))))
              (quote a))

(test-equal "list" (list (quote a) (quote b) (quote c)) (quote (a b c)))
(test-equal "list-empty" (list) (quote ()))

(test-equal "lexical-scope"
  ((lambda (x) (((lambda (x) (lambda () x)) (quote lexical))))
    (quote dynamical))
  (quote lexical))

(define env #f)
((lambda (x y) (set! env (current-environment))) (quote one) (quote two))
(test-equal "current-environment"
            (environment-bindings env)
            (quote ((x . one) (y . two))))

(define env (create-environment))
(set-environment-bindings!
  env
  (list (cons (quote plus) +)
        (cons (quote one) 1)
        (cons (quote two) 2)))
(test-equal "eval" (eval (quote (plus one two)) env) 3)

(test-equal "apply" (apply + (list 1 2 3)) 6)

(test-equal "cond-empty" (cond) #t)
(test-equal "cond-true" (cond (#t (quote a))) (quote a))
(test-equal "cond-false" (cond (#f (quote a))) #t)
(test-equal "cond-short" (cond ((quote a))) (quote a))
(test-equal "cond-two" (cond (#f (quote a)) (#t (quote b))) (quote b))

(test-assert "eq?-symbol-#t" (eq? (quote a) (quote a)))
(test-assert "eq?-symbol-#f" (not (eq? (quote a) (quote b))))
(test-assert "eq?-string-#f" (not (eq? "hello" "hello")))
(test-assert "eq?-number-#f" (not (eq? 5 5)))
(test-assert "eq?-pair-#f" (not (eq? (cons 1 2) (cons 1 2))))
(test-assert "eq?-types-#f" (not (eq? 1 "1")))

(test-assert "eqv?-symbol-#t" (eqv? (quote a) (quote a)))
(test-assert "eqv?-symbol-#f" (not (eqv? (quote a) (quote b))))
(test-assert "eqv?-string-#t" (eqv? "hello" "hello"))
(test-assert "eqv?-number-#t" (eqv? 5 5))
(test-assert "eqv?-char-#t" (eqv? (integer->char 55) (integer->char 55)))
(test-assert "eqv?-char-#f" (not (eqv? (integer->char 55) (integer->char 3))))
(test-assert "eqv?-pair-#f" (not (eqv? (cons 1 2) (cons 1 2))))
(test-assert "eqv?-types-#f" (not (eqv? 1 "1")))

(test-assert "equal?-symbol-#t" (equal? (quote a) (quote a)))
(test-assert "equal?-symbol-#f" (not (equal? (quote a) (quote b))))
(test-assert "equal?-string-#t" (equal? "hello" "hello"))
(test-assert "equal?-number-#t" (equal? 5 5))
(test-assert "equal?-pair-#t" (equal? (cons 1 2) (cons 1 2)))
(test-assert "equal?-types-#f" (not (equal? 1 "1")))
(test-assert "equal?-list-#t"
             (equal?
               (list 1 (quote a) (cons 3 7) "454")
               (list 1 (quote a) (cons 3 7) "454")))

(test-equal "integer->char->integer" (char->integer (integer->char 233)) 233)
(test-assert "char=?-#t" (char=? (integer->char 55) (integer->char 55)))
(test-assert "char=?-#f" (not (char=? (integer->char 55) (integer->char 3))))
(test-equal "string-ref" (char->integer (string-ref "123" 1)) 50)

(test-assert "make-string" (string? (make-string 5)))
(define str (make-string 10))
(string-set! str 3 (integer->char 60))
(test-equal "string-set!" (char->integer (string-ref str 3)) 60)
(define str (make-string 10))
(string-copy! str 0 "abcdefghij" 0 10)
(string-copy! str 0 str 2 10)
(test-equal "string-copy!" str "cdefghijij")
(define str (make-string 10))
(string-copy! str 0 "abcdefghij" 0 10)
(string-copy! str 2 str 0 8)
(test-equal "string-copy!-backwards" str "ababcdefgh")

(define handle (win-open-read "test/data.txt"))
(test-equal "win-get-file-size" (win-get-file-size handle) 6)
(test-assert "win-close-#t" (win-close handle))
(test-assert "win-close-#f" (not (win-close handle)))

(test-end "core")

(test-begin "write" 0)

(write (create-environment))
(write (lambda () 1))
(write (call/cc (lambda (x) (x x))))
(write +)
(write define)
(write if)
(write #t)
(write #f)

(test-end "write")

(error "Expected test error")
