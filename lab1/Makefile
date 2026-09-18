# Student-facing top-level Makefile (shipped as `Makefile` in the starter).
#
# Build each part inside its own subfolder with `make`. When you're ready to
# hand in, run `make submission` here to produce submission.zip.

ZIP = submission.zip

.PHONY: submission clean-submission

# Pack your code into submission.zip (build artifacts are left out).
submission:
	python3 make_submission.py $(ZIP)

clean-submission:
	rm -f $(ZIP)
