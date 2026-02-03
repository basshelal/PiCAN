#pragma once

// TODO a hashmap where the id of the frame is the key and an overwriting ring buffer is the value,
//  the ringbuffer contains the frames with that id (maybe we can trim/compress them before pushing to ringbuffer)
//  the hashmap can be a fixed size (the number of ids we know we are interested in) and each
//  ringbuffer can actually be variable in length (low frequency frames only need a few slots, high frequency
//  will need many to avoid overwriting.
//  This doesn't take the OBD messages into account yet but theoretically these should probably just fit here
//  too
