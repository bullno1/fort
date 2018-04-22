: c immediate next-char ;

: [compile] immediate compile-only compile ;

: " c " [compile] scan-until-char ;
