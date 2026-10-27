// SPDX-License-Identifier: GPL-2.0-only
package mb86h47

import (
	"fmt"
	"os"
	"testing"
)

// TestExport is build-time only: this package contains no USB runtime.
func TestExport(t *testing.T) {
	path := os.Getenv("TORNE_GENERATED_HEADER")
	if path == "" {
		t.Skip("TORNE_GENERATED_HEADER is unset")
	}
	out, err := os.Create(path)
	if err != nil { t.Fatal(err) }
	defer func() { if err := out.Close(); err != nil { t.Error(err) } }()
	fmt.Fprintln(out, `// Copyright 2026 Till0196
// SPDX-License-Identifier: GPL-2.0-only
// Generated from coax 2d9f2c263a3b080ecef3a8ede1eb86c6afa48405.
// GPLv2 branch selected. Regenerate with winusb/tools/torne_codegen/generate.py.
#pragma once
#include <array>
#include <cstdint>
namespace px4::torne::programs {
struct HandlerPatch { std::uint32_t address, original, replacement; };`)
	words := func(name string, code []uint32) {
		fmt.Fprintf(out, "inline constexpr std::array<std::uint32_t, %d> %s = {\n", len(code), name)
		for i, word := range code {
			fmt.Fprintf(out, "0x%08xu,", word)
			if i % 8 == 7 { fmt.Fprintln(out) }
		}
		fmt.Fprintln(out, "\n};")
	}
	for i, a := range addressings {
		words(fmt.Sprintf("addressing%d", i), a.code)
		fmt.Fprintf(out, "inline constexpr std::array<HandlerPatch, %d> patches%d = {{\n", len(a.words), i)
		for _, word := range a.words {
			fmt.Fprintf(out, "{0x%08xu,0x%08xu,0x%08xu},\n", word.address, word.original, word.widened)
		}
		fmt.Fprintln(out, "}};")
	}
	for _, item := range []struct { name string; build func() ([]uint32, error) }{
		{"core0", buildCore0(core0Ring{base: ringBase, size: ringSize, writeOffset: engineBase+0x2038, readLimit: engineBase+0x202c, margin: 8})},
		{"core1", buildCore1}, {"trapRecorder", buildTrapRecorder}, {"trapStubs", buildTrapStubs},
	} {
		code, err := item.build()
		if err != nil { t.Fatal(err) }
		words(item.name, code)
	}
	fmt.Fprintln(out, "} // namespace px4::torne::programs")
}
