		ORIG 100
TEMP,	0
LOOPN,	0

FOUND,	HLT

START,
		CLA CLL CMA CML RAR IAC
		CLA CLL CMA CML RTL IAC
		CLA SMA SZA SNL RSS
		NOP
		

		DCA	TEMP	/ SAVE A
 		TAD	N	
 		CMA IAC		/ -N FOR ISZ
 		DCA	LOOPN	
/	
LOOP,	TAD	X	
 		CMA IAC		/ -X
 		TAD	TEMP	/ A-X
 		SNA CLA		/ SKIP IF NOT EQUAL, CLEAR A
 		JMP	FOUND	/ FOUND IT
 		ISZ	LOOP	/ MODIFY ADDRESS OF X
 		ISZ	LOOPN	/ TEST END OF LOOP
 		JMP	LOOP	
 		NOP	        / NOT FOUND IN LIST
		IOT 4,3
		HLT
		
N,		17
X,		N
		END START