# Choose one of algorithms:
# rswa - Random Sliding Windows Approximation (random_seed and random_range are activated automatically). Shuffle not needed
# scan - full (or window) scan all possible combinations (can randomize starting vector with random_seed and random_range)

# Method parameters
# Recommended: method=window-scan, accept_reseed=1, random_seed=1, late_require=1, s_len=1, random_range=0
# Full scan: method=window-scan, accept_reseed=0, random_seed=0, s_len>c_len-3

rules_file = rules.csv # Specify which file contains all rules
hsp_file = harm.csv # Harmonic sequence penalties
rule_set = 1 # Specify which set of rules to use (see rules.xlsx)

#method = window-scan # This method is fastest random generator with s_len=1, accept_reseed=1, random_seed=1
method = swa # This method is mainly used for testing (same algorithm used as in CA1)

accept_reseed = 1 # After accepting first result reseed (if random_seed) and choose new range (if random_range)
random_seed = 1 # Seed melody with random numbers. This ensures giving different results if generation is very slow.
random_range = 0 # Limit scanning to one of possible fast-scan ranges (automatically enabled for RSWA)
late_require = 1 # Allow not-last scan window to have no needed flags or strict when strict=0 (blocked flags are always disallowed). This calculation usually requires more time
s_len = 1 # Maximum number of measures to full scan. Higher values are slow to compute
t_cnt = 1000 # Maximum number of steps generated

shuffle = 0 # If you want to shuffle all canti after generation (can shuffle up to 32000 canti)

# Main parameters
c_len = 13 # Number of measures in each cantus. Usually 9 to 11 for cantus
key = Am # Music key to use for generation
first_note = A5 # Starting note of each cantus
last_note = A5 # Ending note of each cantus
random_key = 0 # Allow generator to select random key
tempo = 100-130 # Tempo range
cantus_high = 0 # Set to 1 to consider cantus to be higher voice
transpose_back = 1 # Set to 1 to transpose generated melody closer to initial first note

# Random SWA
approximations = 500 # Maximum number of approximations to run if rpenalty decreases
swa_steps = 3 # Size of Sliding Window Approximation algorithm window in steps
correct_range = 7 # Maximum interval allowed between each source and corrected note
rpenalty_accepted = 0 # Maximum accepted rule penalty for RandomSWA

# Rule parameters
fill_steps_mul = 2 # Multiply number of notes between leap notes to get steps for filling (+2)
first_steps_tonic = 3 # Number of first steps, which must contain tonic note (will increase by one if cantus is longer than 10 notes)

# Technical parameters
midifile_out_mul = 2 # All notes length is multiplied by this number before exporting
show_severity = 1 # =1 to show severity in square brackets in comments to notes (also when exporting to MIDI file)
calculate_correlation = 0 # Enables correlation calculation algorithm. Slows down generation. Outputs to cf1-cor.csv
calculate_blocking = 1 # Enables blocking flags calculation algorithm. Slows down generation.
calculate_stat = 0 # Enables flag statistics calculation algorithm. Slows down generation.
calculate_ssf = 0 # Enables SWA stuck flags statistics calculation algorithm.
best_rejected = 0 # Show best rejected results if rejecting more than X ms. Requires blocking or stat to work. Set to 0 to disable. Slows down generation

midifile_export_marks = 0 # Set to export marks (harmony) to midifile lyrics
midifile_export_comments = 0 # Set to export comments (violations) to midifile lyrics
