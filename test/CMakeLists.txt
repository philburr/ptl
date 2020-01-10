
function(unittest name)
	add_executable(${name} ${name}.cpp)
	target_link_libraries(${name} PRIVATE ptl unittest)
	target_compile_options(${name} PRIVATE -march=native)
endfunction()

unittest(list_ut)
unittest(integer_ut)
unittest(strong_type_ut)
unittest(scope_guard_ut)
unittest(expected_ut)
unittest(packed_ptr_ut)
