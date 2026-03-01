This is my current side project im doing for fun

Right now, i am just working on getting the CPU working (ARM7TDMI) which is proving itself to be a challenge.

I currently have all the THUMB instructions fully working and tested

### UPDATE:
About 80% of arm instructions are implemented, just missing test suite fully passing on following now
- arm_cdp					
- arm_ldrsb_ldrsh
- arm_mcr_mrc	
- arm_mrs
- arm_msr_imm	
- arm_msr_reg	
- arm_mul_mla	
- arm_mull_mlal
- arm_stc_ldc
- arm_swi			
- arm_swp

so basically most software interupt handling, multiplying and a few load store instrs
