
project = "MQSS Quantum Compilation Suite"
extensions = ["myst_parser", "sphinx_multiversion"]   # enables Markdown support + versioned docs
html_theme = "furo"            # clean modern theme
html_static_path = ["_static"]
templates_path = ["_templates"]

# -- sphinx-multiversion ------------------------------------------------
# Which refs to build docs for: released version tags (vX.Y.Z) only. Docs
# are published on release, not on every commit to develop -- branch builds
# are disabled entirely (a regex matching no real branch name), not just
# left out of the CI trigger, since sphinx-multiversion rescans every
# matching ref on every run regardless of what triggered that run.
smv_tag_whitelist = r'^v\d+\.\d+\.\d+$'
smv_branch_whitelist = r'^$'
smv_released_pattern = r'^refs/tags/v.*$'
smv_outputdir_format = '{ref.name}'

# Render the version switcher in the sidebar on every page, alongside furo's
# default sidebar components.
html_sidebars = {
    "**": [
        "sidebar/brand.html",
        "sidebar/search.html",
        "versioning.html",
        "sidebar/scroll-start.html",
        "sidebar/navigation.html",
        "sidebar/ethical-ads.html",
        "sidebar/scroll-end.html",
    ]
}
