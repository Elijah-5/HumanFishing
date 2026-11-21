// Encode captured gameplay PNGs with macOS ImageIO; no third-party tools required.
// swift -module-cache-path /private/tmp/hf-swift-cache Scripts/encode_readme_gif.swift FRAMES OUTPUT.gif [WIDTH] [TRIM_START_SECONDS]
import Foundation
import ImageIO
import UniformTypeIdentifiers

let args = CommandLine.arguments
guard args.count >= 3 else { fatalError("Usage: encode_readme_gif.swift FRAMES OUTPUT.gif [WIDTH] [TRIM_START_SECONDS]") }
let directory = URL(fileURLWithPath: args[1], isDirectory: true)
let output = URL(fileURLWithPath: args[2])
let width = args.count > 3 ? Int(args[3])! : 960
let allFrames = try FileManager.default.contentsOfDirectory(at: directory, includingPropertiesForKeys: nil)
    .filter { $0.pathExtension.lowercased() == "png" }.sorted { $0.lastPathComponent < $1.lastPathComponent }
guard !allFrames.isEmpty else { fatalError("No captured PNGs in \(directory.path)") }
let allTimes = try allFrames.map { try $0.resourceValues(forKeys: [.contentModificationDateKey]).contentModificationDate! }
let trim = args.count > 4 ? Double(args[4])! : 0
guard let start = allTimes.firstIndex(where: { $0.timeIntervalSince(allTimes[0]) >= trim }) else { fatalError("Trim exceeds clip length") }
let frames = Array(allFrames[start...]), captureTimes = Array(allTimes[start...])
guard let destination = CGImageDestinationCreateWithURL(output as CFURL, UTType.gif.identifier as CFString, frames.count, nil)
    else { fatalError("Cannot create \(output.path)") }
CGImageDestinationSetProperties(destination, [kCGImagePropertyGIFDictionary: [kCGImagePropertyGIFLoopCount: 0]] as CFDictionary)
for (index, frame) in frames.enumerated() {
    try autoreleasepool {
        guard let source = CGImageSourceCreateWithURL(frame as CFURL, nil),
              let image = CGImageSourceCreateThumbnailAtIndex(source, 0, [
                kCGImageSourceCreateThumbnailFromImageAlways: true,
                kCGImageSourceThumbnailMaxPixelSize: width,
                kCGImageSourceShouldCacheImmediately: true
              ] as CFDictionary) else { throw NSError(domain: "GIF", code: 1) }
        // Screenshot readback can reduce capture rate. Preserve the recorded timing.
        let duration = index == frames.count - 1 ? 0.8 : max(0.02, captureTimes[index + 1].timeIntervalSince(captureTimes[index]))
        CGImageDestinationAddImage(destination, image, [kCGImagePropertyGIFDictionary: [
            kCGImagePropertyGIFDelayTime: duration,
            kCGImagePropertyGIFUnclampedDelayTime: duration
        ]] as CFDictionary)
    }
}
guard CGImageDestinationFinalize(destination) else { fatalError("GIF encoding failed") }
let bytes = try FileManager.default.attributesOfItem(atPath: output.path)[.size] as! NSNumber
print("\(output.lastPathComponent): \(frames.count) frames, \(width)px, \(bytes) bytes")
