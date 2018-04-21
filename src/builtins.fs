: ; immediate compile-only
	['] exit [ compile ] compile def-end
	[ ' [ compile ] exit
	[ def-end

: return-to-native
	switch [ def-end

: c immediate next-char ;

: [compile] immediate compile-only compile ;

: " c " [compile] scan-until-char ;
