package src

import "core:fmt"
import "core:os"
import "core:strings"
import "core:strconv"
import "core:time"
import "core:math/linalg/glsl"


read_file_by_lines_in_whole :: proc(filepath: string) -> [dynamic]f32 {
	results : [dynamic]f32

	data, ok := os.read_entire_file(filepath, context.allocator)
	if !ok {
		// could not read file
        fmt.println("Could not read file ", filepath)
		return results
	}
	defer delete(data, context.allocator)

	it := string(data)
	for line in strings.split_lines_iterator(&it) {
		// process line
		tokens := strings.split(line, " ")
		for token in tokens{
			// Process token.
			value, ok := strconv.parse_f32(token)
			if ok {
				append(&results, value)
			}
		}
	}

	return results
}

main :: proc() {
	timings : [dynamic]time.Duration

	// Benchmark: load up the file I need.
	matrices : [dynamic]glsl.mat4
	{
		sw := time.Stopwatch{}
		time.stopwatch_start(&sw)

		// Load floats from file.
		tokens_as_floats := read_file_by_lines_in_whole("assets/bunch_o_numbers.txt")
		assert(len(tokens_as_floats) == 634)  // @NOTE: From the cpp version result!

		// Move to matrices list.
		for i := 16; i < len(tokens_as_floats); i += 16 {
			new_matri := glsl.mat4{ tokens_as_floats[i - 16],
                                    tokens_as_floats[i - 15],
                                    tokens_as_floats[i - 14],
                                    tokens_as_floats[i - 13],

                                    tokens_as_floats[i - 12],
                                    tokens_as_floats[i - 11],
                                    tokens_as_floats[i - 10],
                                    tokens_as_floats[i - 9],

                                    tokens_as_floats[i - 8],
                                    tokens_as_floats[i - 7],
                                    tokens_as_floats[i - 6],
                                    tokens_as_floats[i - 5],

                                    tokens_as_floats[i - 4],
                                    tokens_as_floats[i - 3],
                                    tokens_as_floats[i - 2],
                                    tokens_as_floats[i - 1] }
			append(&matrices, new_matri)
		}

		// Finish.
		dt := time.stopwatch_duration(sw)
		append(&timings, dt)
	}

	// Perform matrix multiplication.
	{
		sw := time.Stopwatch{}
		time.stopwatch_start(&sw)

		cum_val := glsl.identity(glsl.mat4)
		for matri in matrices {
			cum_val *= matri
		}

		// Finish.
		dt := time.stopwatch_duration(sw)
		append(&timings, dt)
	}

	// Results.
	fmt.printf("File loading time (sec): %0.10f\n" +
               "Matrix multi time (sec): %0.10f\n",
			   f64(timings[0]) / 1_000_000_000.0,
			   f64(timings[1]) / 1_000_000_000.0)
}
