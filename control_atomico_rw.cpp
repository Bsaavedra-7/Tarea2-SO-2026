/* 
modo atomic

implementa la politica de prioridad de escritores sin mutex,
 usando mascaras de bits, bucles de espera con cas y contadores locales de intetos fallidos.

*/