//  _
// | |
// | |_   _ __   ___    ___
// | __| | '__| / _ \  / _ \     An opinionated tree command
// | |_  | |   |  __/ |  __/     that's _just_ how I like it!
//  \__| |_|    \___|  \___|
//
//                     ; ; ;
//                   ;        ;  ;     ;;    ;
//                ;                 ;         ;  ;
//                                ;
//                               ;                ;;
//               ;          ;            ;              ;
//               ;            ';,        ;               ;
//               ;              'b      *
//                ;              '$    ;;                ;;
//               ;    ;           $:   ;:               ;
//             ;;      ;  ;;      *;  @):        ;   ; ;
//                          ;     :@,@):   ,;**:'   ;
//              ;      ;,         :@@*: ;;**'      ;   ;
//                       ';o;    ;:(@';@*"'  ;
//               ;  ;       'bq,;;:,@@*'   ,*      ;  ;
//                          ,p$q8,:@)'  ;p*'      ;
//                   ;     '  ; '@@Pp@@*'    ;  ;
//                    ;  ; ;;    Y7'.'     ;  ;
//                              :@):.
//                             .:@:'.
//                           .::(@:.

package main

import (
	"cmp"
	"fmt"
	"github.com/docopt/docopt-go"
	"github.com/fatih/color"
	"io/fs"
	"io/ioutil"
	"os"
	"path/filepath"
	"slices"
)

var DocUsage = `
tree

Usage:
  tree [-drsfD] [-L level] [<path>]
  tree -h | --help

Options:
  -d         Show directories only
  -L level   Descend only 'level' directories deep
  -r         Sort in reverse alphabetic order
  -s         Print a summary of directories and files at the end
  -f         Print the full path of each file
  -D         Dumb mode (recurses into every directory and lists every file)
  -h --help  This help message
`

// Configuration type to represent docopt options
type TreeConfig struct {
	Path            string `docopt:"<path>"`
	MaxDepth        int    `docopt:"-L"`
	DirectoriesOnly bool   `docopt:"-d"`
	ReverseSort     bool   `docopt:"-r"`
	ShowSummary     bool   `docopt:"-s"`
	FullPath        bool   `docopt:"-f"`
	DumbMode        bool   `docopt:"-D"`
}

var SkippableDirectories = []string{
	".git",         // git
	"zig-cache",    // Zig
	"node_modules", // Node
	"target",       // Rust, Clojure, etc
	".terraform",   // Terraform
}

func skippable(absPath string) bool {
	basePath := filepath.Base(absPath)

	for _, d := range SkippableDirectories {
		if basePath == d {
			return true
		}
	}

	return false
}

// Helpers for the pretty output
var LineHorizontal string = "\u2500" // ─
var LineVertical string = "\u2502"   // │
var LineMiddle string = "\u251c"     // ├
var LineLast string = "\u2514"       // └

var IndentMiddleItem string = LineMiddle + LineHorizontal + LineHorizontal
var IndentLastItem string = LineLast + LineHorizontal + LineHorizontal

type treeEntity struct {
	absPath  string
	fileInfo os.FileInfo
}

func (e treeEntity) echoItem(indent string, relPath string) error {
	modeBits := e.fileInfo.Mode()

	if e.fileInfo.IsDir() {
		fmt.Print(indent + " ")
		dirColor := color.New(color.Bold, color.FgBlue)
		dirColor.Println(relPath)
	} else {
		if (modeBits & fs.ModeSymlink) != 0 {
			linkPath, err := os.Readlink(e.absPath)

			if err != nil {
				return err
			}

			fmt.Print(indent + " ")
			srcColor := color.New(color.Bold, color.FgRed)
			srcColor.Print(relPath)
			fmt.Print(" -> ")
			lnkColor := color.New(color.FgCyan)
			lnkColor.Println(linkPath)
		} else if (modeBits & fs.ModePerm & 0o111) != 0 {
			fmt.Print(indent + " ")
			execColor := color.New(color.Bold)
			execColor.Println(relPath)
		} else {
			fmt.Println(indent + " " + relPath)
		}
	}

	return nil
}

type crawlResult struct {
	numFiles   uint
	numFolders uint
}

func (c crawlResult) PrintSummary() {
	fmt.Print("\n")

	if c.numFolders == 1 {
		fmt.Print("1 folder")
	} else {
		fmt.Printf("%d folders", c.numFolders)
	}

	fmt.Print(", ")

	if c.numFiles == 1 {
		fmt.Print("1 file")
	} else {
		fmt.Printf("%d files", c.numFiles)
	}

	fmt.Print("\n")
}

func crawl(path string, level int, prefix string, config TreeConfig) (*crawlResult, error) {
	var rv crawlResult

	// Reached max depth, stop recursing
	if config.MaxDepth > 0 && level == config.MaxDepth {
		return &rv, nil
	}

	// Read the current directory
	var entities []treeEntity

	pathContents, err := ioutil.ReadDir(path)

	if err != nil {
		// If we can't read a directory, just move on. I'd rather not have the whole program exit because
		// trying to read something from /dev/fd/ failed...

		//return nil, err
	}

	for _, file := range pathContents {
		absPath := path + "/" + file.Name()

		if (config.DirectoriesOnly && file.IsDir()) || !config.DirectoriesOnly {
			entities = append(entities, treeEntity{
				absPath:  absPath,
				fileInfo: file,
			})
		}

	}

	// Sort directories to the top first, then sort by name
	slices.SortFunc(entities, func(a, b treeEntity) int {
		if a.fileInfo.IsDir() && !b.fileInfo.IsDir() {
			return -1
		}

		if !a.fileInfo.IsDir() && b.fileInfo.IsDir() {
			return 1
		}

		return cmp.Compare(a.absPath, b.absPath)
	})

	// Reverse, if specified
	if config.ReverseSort {
		slices.Reverse(entities)
	}

	for i, v := range entities {
		// Calculate new indentation string for output
		indent := prefix + IndentMiddleItem
		if i == len(entities)-1 {
			indent = prefix + IndentLastItem
		}

		relPath := v.fileInfo.Name()

		if config.FullPath {
			relPath = path + "/" + v.fileInfo.Name()
		}

		err := v.echoItem(indent, relPath)

		if err != nil {
			return nil, err
		}

		// If it's a directory, figure out the next prefix string, and then recurse
		if v.fileInfo.IsDir() {
			newPrefix := prefix + LineVertical + "   "
			if i == len(entities)-1 {
				newPrefix = prefix + "   "
			}

			rv.numFolders += 1

			if config.DumbMode || !skippable(v.absPath) {
				dirResult, err := crawl(v.absPath, level+1, newPrefix, config)

				if err != nil {
					return nil, err
				}

				rv.numFolders += dirResult.numFolders
				rv.numFiles += dirResult.numFiles
			}
		} else {
			rv.numFiles += 1
		}
	}

	return &rv, nil
}

func main() {
	var config TreeConfig
	arguments, err := docopt.ParseDoc(DocUsage)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	arguments.Bind(&config)

	// If not path was specified, use the current directory
	if len(config.Path) == 0 {
		cwd, err := os.Getwd()

		if err != nil {
			fmt.Println(err)
			os.Exit(1)
		}

		config.Path = cwd
	}

	// Strip trailing slashes
	for config.Path[len(config.Path)-1] == '/' {
		config.Path = config.Path[:len(config.Path)-1]
	}

	fmt.Println(config.Path)
	rv, err := crawl(config.Path, 0, "", config)

	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}

	if config.ShowSummary {
		rv.PrintSummary()
	}
}
