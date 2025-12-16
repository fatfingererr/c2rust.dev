+++
template = "landing.html"
title = "Home"

[extra]
version = "v0.1.0"

# Hero Section
# The main section at the top of the page.
[extra.hero]
title = "impl<C> Fun for Rust"
badge = "🌐 This blog is currently written in Chinese"
description = "Ricky 的 Rust 主題技術部落格"
image = "/images/landing.jpg" # Background image
cta_buttons = [
    { text = "cargo read", url = "/get-started/installation/", style = "primary" },
    { text = "View on GitHub", url = "https://github.com/fatfingererr/c2rust.dev", style = "secondary" },
]

# Features Section
# A grid to highlight key features.
# Configure the section title and description
[extra.features_section]
title = "Essential Features"
description = "Clean, minimal, and focused on content"

[[extra.features_section.features]]
title = "Documentation Friendly"
desc = "Provides a clean writing experience for documentation."
icon = "fa-solid fa-book"

[[extra.features_section.features]]
title = "Simple Design"
desc = "A theme that pursues minimalism."
icon = "fa-solid fa-minimize"

# Trust Section
# Display logos of companies or projects that trust you.
[extra.trust_section]
title = "Trusted by the Best"
logos = [
    { src = "/images/logo1.svg", alt = "Company One" },
    { src = "/images/logo2.svg", alt = "Company Two" },
]

# Showcase Section
# Display tabbed content with images or text.
# Each tab can show either an image or text content (title + description).
[extra.showcase_section]
title = "Theme Showcase"
subtitle = "Explore different aspects of Goyo theme"

[[extra.showcase_section.tabs]]
name = "Documentation"
title = "Clean Documentation"
description = "Experience beautiful and readable documentation pages with Goyo's minimalist design approach."
image = "/writing/shortcodes/gallery/images/image1.jpeg"

[[extra.showcase_section.tabs]]
name = "Customization"
title = "Easy Customization"
description = "Customize your site with simple configuration options."
image = "/writing/shortcodes/gallery/images/image5.jpeg"

[[extra.showcase_section.tabs]]
name = "Multilingual"
title = "Multilingual Support"
description = "Built-in support for multiple languages."
image = "/writing/shortcodes/gallery/images/image6.jpeg"

# Social Proof Section
# Showcase testimonials from your users.
[extra.social_proof_section]
title = "What Our Users Say"

[[extra.social_proof_section.testimonials]]
author = "Jane Doe"
role = "Developer at TechCorp"
quote = "Goyo has transformed our documentation process. It's simple, elegant, and incredibly fast."
avatar = "/images/avatars/jane.png"

[[extra.social_proof_section.testimonials]]
author = "John Smith"
role = "Project Manager at Innovate LLC"
quote = "The best Zola theme for documentation out there. The setup was a breeze."
avatar = "/images/avatars/john.png"

# Final Call to Action Section
# A final prompt for users before the footer.
# You can also add an image field to show an image above the CTA text.
[extra.final_cta_section]
title = "Ready to Get Started?"
description = "Begin your journey with Goyo today and create beautiful documentation with ease."
button = { text = "Start Now", url = "/get-started/installation/" }
image = "/images/contribute.png" # (Optional) Image above the CTA section
+++