package main

/*
#cgo darwin CXXFLAGS: -DSCREENSHOT_COCOA -std=c++11
#cgo darwin LDFLAGS: -framework WebKit

#define SCREENSHOT_HEADER
#include "screenshot.h"

#include <stdlib.h>
#include <stdint.h>

*/
import "C"
import (
	"os"
	"fmt"
	"unsafe"
	"path/filepath"
)

func main() {
	exe, err := os.Executable()
	if err != nil {
		os.Exit(1)
	}

	fmt.Println("Start")
	w := C.int(640)
	h := C.int(480)
	title := C.CString("Sample")
	defer C.free(unsafe.Pointer(title))

	filePath := filepath.Join(filepath.Dir(exe), "result.png")
	path := C.CString(filePath)
	defer C.free(unsafe.Pointer(path))

	C.screenshot_run(w, h, title, path)
}
