import sys
import math

def calculate_timecode(frame_number, framerate):
    # Calculate drop frames
    drop_frames = round(framerate * 0.066666)

    # Calculate frame counts
    frames_per_hour = round(framerate * 60 * 60)
    frames_per_24_hours = frames_per_hour * 24
    frames_per_10_minutes = round(framerate * 60 * 10)
    frames_per_minute = round(framerate * 60) - drop_frames

    # Handle negative frame numbers
    while frame_number < 0:
        frame_number += frames_per_24_hours

    # Roll over clock after 24 hours
    frame_number = frame_number % frames_per_24_hours

    # Calculate hours, minutes, seconds, and frames
    d = frame_number // frames_per_10_minutes
    m = frame_number % frames_per_10_minutes

    if m > drop_frames:
        frame_number += (drop_frames * 9 * d) + drop_frames * ((m - drop_frames) // frames_per_minute)
    else:
        frame_number += drop_frames * d

    fr_round = round(framerate)
    frames = frame_number % fr_round
    seconds = (frame_number // fr_round) % 60
    minutes = ((frame_number // fr_round) // 60) % 60
    hours = (((frame_number // fr_round) // 60) // 60)

    return f"{hours:02}:{minutes:02}:{seconds:02}:{frames:02}"

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: python test.py <frame_number> <framerate>")
        sys.exit(1)

    frame_number = int(sys.argv[1])
    framerate = float(sys.argv[2])

    timecode = calculate_timecode(frame_number, framerate)
    print(timecode)