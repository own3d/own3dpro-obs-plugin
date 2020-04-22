let process = require('process');
let fs = require('fs');
let path = require('path');

// Initialization
let args = process.argv.slice(2);

// Input File
if (args.length != 1) {
	console.error("Missing file argument.");
	process.exit(1);
}
let input = args[0];

// Load as JSON
let json = JSON.parse(fs.readFileSync(input));

// Create Directories.
let path_output = path.posix.join('.', 'output');
let path_data = path.posix.join(path_output, 'data');
if (!fs.existsSync(path_output)) {
	fs.mkdirSync(path_output);
}
if (!fs.existsSync(path_data)) {
	fs.mkdirSync(path_data);
}

// Parse sources which might contain file information.
function copyRecursiveSync(src, dest) {
	var exists = fs.existsSync(src);
	var stats = exists && fs.statSync(src);
	var isDirectory = exists && stats.isDirectory();
	if (isDirectory) {
		fs.mkdirSync(dest);
		fs.readdirSync(src).forEach(function(childItemName) {
			copyRecursiveSync(path.posix.join(src, childItemName),
							  path.posix.join(dest, childItemName));
		});
	} else {
		fs.copyFileSync(src, dest);
	}
};
function posixify(path) {
	const isExtendedLengthPath = /^\\\\\?\\/.test(path);
	const hasNonAscii = /[^\u0000-\u0080]+/.test(path); // eslint-disable-line no-control-regex

	if (isExtendedLengthPath || hasNonAscii) {
		return path;
	}

	return path.replace(/\\/g, '/');
}

function make_relative_paths(obj) {
	for (let key in obj) {		
		let value = obj[key];
		switch (typeof (value)) {
			case "string": // Only type of interest aside from object.
				let parsed = path.parse(value);
				if (parsed.dir == "") {
					break;
				}
				if (!fs.existsSync(parsed.dir)) {
					break;
				}
				// At this point it's almost surely a file that we can deal with.
				let output = path.posix.join(path_data, path.basename(value));
				copyRecursiveSync(value, output);
				obj[key] = posixify(path.posix.join("<REPLACE|ME>", "data", path.basename(value)));
				break;
			case "object":
				make_relative_paths(obj[key]);
				break;
			default:
				break;
		}
	}
}

for (let value of json.sources) {
	// Above gives us a REFERENCE to the object contained.
	// This is as close to a pointer as we can get.
	make_relative_paths(value.settings);
}
for (let value of json.transitions) {
	// Above gives us a REFERENCE to the object contained.
	// This is as close to a pointer as we can get.
	make_relative_paths(value.settings);
}

fs.writeFileSync(path.join(path_output, 'data.json'), JSON.stringify(json));
