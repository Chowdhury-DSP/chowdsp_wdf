# Combining WDF Elements:

With WDFs, it's pretty common to combine a series
voltage source plus resistor into a "resistive voltage 
source", in order to be able to use a voltage source
as an "adaptable" WDF one-port. However, we can also
combine other WDF elements with the goal being that
the compiler can better optimize the computations needed
for the combined elements.

## Resistor-Capacitor Series

Here we create a combined one-port for a resistor and
capacitor in series. Note that with the combined element
we no longer have direct access to the voltage or current
across either the resistor or capacitor, so this approach
doesn't work great if you need to "probe" either element.

### Underlying Components:

A WDF Resistor is defined by:
$$
R_p = R \\
b = 0
$$

A WDF Capacitor is defined by:
$$
R_p = \frac{T}{2C} \\
b = a[n-1]
$$

And a 3-port WDF Series adaptor:
$$
R_p = R_1 + R_2 \\
b_0 = -a_1 - a_2 \\
b_1 = -\frac{R_1}{R_p} a_0
    + \frac{R_2}{R_p} a_1
    - \frac{R_1}{R_p} a_2\\
b_2 = -\frac{R_2}{R_p} a_0
    - \frac{R_2}{R_p} a_1
    + \frac{R_1}{R_p} a_2
$$

We can simplify this into 1-multiply form. First, we
recognize the similarity between the $b_1$ and $b_2$ terms:
$$
b_1 + b_2 = -\frac{R_1}{R_p} a_0 - \frac{R_2}{R_p} a_0 \\
b_1 + b_2 = -\left(\frac{R_1 + R_2}{R_1 + R_2} a_0 \right) \\
b_1 + b_2 = -a_0 \\
b_2 = -(a_0 + b_1)
$$

So if we can figure out $b_1$ then we can compute $b_2$
with zero multiplications.

$$
b_1 = -\frac{R_1}{R_p} a_0
    + \frac{R_2}{R_p} a_1
    - \frac{R_1}{R_p} a_2 \\
b_1 = -\frac{R_1}{R_p} a_0
    + \frac{R_2}{R_p} a_1
    - \frac{R_1}{R_p} a_2
    + \frac{R_1}{R_p} a_1
    - \frac{R_1}{R_p} a_1 \\
b_1 = -\frac{R_1}{R_p} (a_0 + a_1 + a_2)
    + \frac{R_2}{R_p} a_1
    + \frac{R_1}{R_p} a_1 \\
b_1 = a_1 -\frac{R_1}{R_p} (a_0 + a_1 + a_2)
$$

So the full 1-multiply form is:
$$
R_p = R_1 + R_2 \\
b_0 = -a_1 - a_2 \\
b_1 = a_1 -\frac{R_1}{R_p} (a_0 + a_1 + a_2) \\
b_2 = -(a_0 + b_1)
$$

### Combining...

Now we can combine these elements. We'll use port (1)
of the series adaptor for the capacitor, and port (2)
for the resistor. Remember that $a$ and $b$ are being
described from the reference point of the given element,
so $a_1$ for the series adaptor is actually $b$ from the 
capacitor. To avoid duplicated names, we'll use $b_c$ and
$a_c$ for the variables from the perspective of the capacitor
and $b_r$ and $a_r$ for the resistor.

First we sub in $b_c$ and $b_r$ for $a_1$ and $a_2$:
$$
R_p = \frac{T}{2C} + R \\
R_i = \frac{\frac{T}{2C}}{\frac{T}{2C} + R}
    = \frac{T}{T + 2 R C} \\
b_0 = -b_c - b_r \\
b_1 = a_c[n-1] - R_i (a_0 + b_c + b_r) \\
b_2 = -(a_0 + b_1)
$$

And then we can sub in the definitions of $b_c$ and $b_r$:
$$
b_0 = -a_c[n-1] - 0 \\
b_1 = a_c[n-1] - R_i (a_0 + a_c[n-1] + 0) \\
b_2 = -(a_0 + b_1)
$$

Now we can sub in $a_c$ and $a_r$ for $b_1$ and $b_2$:
$$
b_0 = -a_c[n-1] \\
a_c = a_c[n-1] - R_i (a_0 + a_c[n-1]) \\
a_r = -(a_0 + a_c)
$$

We no longer need $a_r$, and we can rename $a_c$ to $z$.
We can also just use $b$ and $a$ instead of $b_0$ and $a_0$.
$$
b_0 = -z[n-1] \\
z = z[n-1] - R_i (a_0 + z[n-1])
$$

## Resistor-Capacitor Parallel:

### Underlying Components:

Now we can do the same thing, starting with a 3-port
parallel adaptor:

$$
G_p = G_1 + G_2 \\
b_0 = \frac{G_1}{G_p} a_1 + \frac{G_2}{G_p} a_2 \\
b_1 = a_0 - \frac{G_2}{G_p} a_1 + \frac{G_2}{G_p} a_2 \\
b_2 = a_0 + \frac{G_2}{G_p} a_1 - \frac{G_2}{G_p} a_2
$$

Again, we find a simple relationship between $b_1$ and $b_2$:
$$
b_{diff} = a_2 - a_1 \\
b_1 = a_0 + \frac{G_2}{G_p} b_{diff} \\
b_2 = a_0 - \frac{G_1}{G_p} b_{diff} \\
b_1 - b_2 = \frac{G_2}{G_p} b_{diff}+\frac{G_1}{G_p} b_{diff}\\
b_1 - b_2 = b_{diff}
$$

Then solving for $b_0$:
$$
b_0 = \frac{G_1}{G_p} a_1
    + \frac{G_2}{G_p} a_2
    + \frac{G_1}{G_p} a_2
    - \frac{G_1}{G_p} a_2 \\
b_0 = \frac{G_1}{G_p} a_2 + \frac{G_2}{G_p} a_2
    + \frac{G_1}{G_p} (a_1 - a_2) \\
b_0 = a_2 - \frac{G_1}{G_p} b_{diff}
$$

So the full 1-multiply parallel adaptor:
$$
G_p = G_1 + G_2 \\
b_{diff} = a_2 - a_1 \\
b_0 = a_2 - \frac{G_1}{G_p} b_{diff} \\
b_1 = b_2 + b_{diff} \\
b_2 = a_0 - \frac{G_1}{G_p} b_{diff} = b_0 - a_2 + a_0
$$

### Combining...

Again, we can combine the elements with the capacitor
at port (1), and resistor at port (2):

$$
G_p = \frac{2C}{T} + \frac{1}{R} \\
G_i = \frac{2 C R}{2 C R + T} \\
b_{diff} = b_r - b_c \\
b_0 = b_r - G_i b_{diff} \\
b_1 = b_2 + b_{diff} \\
b_2 = b_0 - b_r + a_0
$$

$b_{diff}$ condenses very nicely, to $-a_c[n-1]$, so:

$$
b_0 = 0 + G_i a_c[n-1] \\
b_1 = b_2 - a_c[n-1] \\
b_2 = b_0 - 0 + a_0
$$

Then substituting in $b_1 \rarr a_c$ and $b_2 \rarr a_r$:
$$
b_0 = G_i a_c[n-1] \\
a_c = a_r - a_c[n-1] \\
a_r = b_0 + a_0 \\
a_c = b_0 + a_0 - a_c[n-1]
$$

So the final derivation:
$$
b = G_i z \\
z = b + a - z[n-1]
$$

## Resistive Voltage Source + Capacitor (Series)

### Underlying Components:

A WDF Resistive Voltage source is defined:
$$
R_p = R \\
b = V
$$

### Combining...

Now we can start again with the 1-multiply series adaptor:

$$
R_p = R_1 + R_2 \\
b_0 = -a_1 - a_2 \\
b_1 = a_1 -\frac{R_1}{R_p} (a_0 + a_1 + a_2) \\
b_2 = -(a_0 + b_1)
$$

And start subbing in:
$$
R_p = \frac{T}{2C} + R \\
R_i = \frac{T}{T + 2 R C} \\
b_0 = -a_c[n-1] - V \\
a_c = a_c[n-1] - R_i (a_0 + a_c[n-1] + V)
$$

Note that as with the previous series combination,
we can ignore the $a_r$ term. We can simplify $a_c$
a little bit more:
$$
a_c = a_c[n-1] - R_i (a_0 - b)
$$

In final form:
$$
b = -z - V \\
z = z - R_i (a - b)
$$
