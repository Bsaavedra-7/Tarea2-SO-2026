/* 
modo atomic

Declara la clase ControlAtomicoRW, derivada de ControlRW.

tiene solo una palabra atomica: std::atomic<std::uint64_t> 
y el static_assert que garantiza que sea lock-free


*/