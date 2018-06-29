package main

import (
	"fmt"
	"image"
	"image/png"
	"os"

	cf "github.com/lucasb-eyer/go-colorful"
)

// This program outputs a 360*100 HSV rectangle; Hue on X-axis (0-360), Value on Y-axis (1-0)
func main() {
	ret := image.NewRGBA(image.Rect(0, 0, 360, 100))

	for x := 0; x <= 360; x++ {
		for y := 0; y <= 100; y++ {
			h := float64(x)
			v := float64(100-y) / 100
			ret.Set(x, y, cf.Hsv(h, 1.0, v))
		}
	}

	// Output the finished image
	file, err := os.Create("hsv.png")
	if err != nil {
		fmt.Fprintln(os.Stderr, err.Error())
		os.Exit(1)
	}
	defer file.Close()
	png.Encode(file, ret)
}
