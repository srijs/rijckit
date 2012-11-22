# Rijckit

A handwritten, low-level lexer for c-like languages. Still in early development.

## Introduction

### What's that? An icelandic delicacy?

Quite, quite, my friend. When Rijckit is grown up, it wants to be a analysis-tool for the C family of languages.

In the meantime, it can already lex a bunch of C, with more (preprocessor, parser) soon to follow.
Rijckit is open source, so you can verify that it is indeed lovely handcrafted and – if you wish – contribute some of your spare time to it, just like I like to do.

### That sounds nice. What does it do exactly, so far?

Rijckit works (so far I can tell – unit test are another thing to follow) already quite well. It lexes many C programs completely, including, but not interpreting, preprocessor statements. But unfortunately it is very simple-minded with numbers – just decimal digits will work for now.