: ackermann ( m n -- a )
    over 0= if nip 1+ else
    dup 0=  if drop 1- 1 recurse else
               1- over ( m n-1 m )
               1- -rot ( m-1 m n-1 )
               recurse recurse
    \ " wat" .  .s
    then then ;

3 12 ackermann
.s
