Features
- **Vector-like Shape Management:** Rather than using a destructive drawing system, shapes are stored as structures (Vector representation). You can add, delete, or modify shapes independently at any time, and the canvas will cleanly re-render.
- **Draw Canvas:** Renders a $60 \times 20$ grid of characters. Unoccupied space displays as `_`, and shapes are drawn with `*`.
- **4 Primitive Shape Tools:**
  - **Line:** Bresenham's Line Algorithm.
  - **Circle:** Midpoint (Bresenham) Circle Drawing Algorithm.
  - **Rectangle:** Draws boundary lines for height and width.
  - **Triangle:** Connects three coordinates.
