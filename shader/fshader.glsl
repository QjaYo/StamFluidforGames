#version 330 core

in vec2 vTexCoord;
out vec4 FragColor;

uniform sampler2D uDensTex;
uniform sampler2D uHouseTex;
uniform int uRenderMode;
uniform float uViewportAspect;

float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float noise12(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash12(i);
    float b = hash12(i + vec2(1.0, 0.0));
    float c = hash12(i + vec2(0.0, 1.0));
    float d = hash12(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.5;
    for (int i = 0; i < 4; i++)
    {
        value += amplitude * noise12(p);
        p = p * 2.05 + vec2(17.3, 9.1);
        amplitude *= 0.5;
    }
    return value;
}

float cloudMask(vec2 uv, float aspect)
{
    vec2 p = vec2(uv.x * aspect, uv.y);
    float lowBand = smoothstep(0.10, 0.24, uv.y) * (1.0 - smoothstep(0.64, 0.78, uv.y));
    float midBand = smoothstep(0.30, 0.46, uv.y) * (1.0 - smoothstep(0.70, 0.84, uv.y));

    float softCloud = smoothstep(0.50, 0.76, fbm(vec2(p.x * 2.05 + 0.6, p.y * 5.0 - 0.7))) * lowBand;
    float wisps = smoothstep(0.56, 0.80, fbm(vec2(p.x * 5.1 - 1.4, p.y * 11.5 + 3.0))) * midBand;
    return clamp(softCloud * 0.34 + wisps * 0.18, 0.0, 0.48);
}

vec3 renderNightSky(vec2 uv)
{
    vec3 horizon = vec3(0.020, 0.025, 0.055);
    vec3 zenith = vec3(0.003, 0.008, 0.026);
    float height = smoothstep(0.0, 1.0, uv.y);
    vec3 sky = mix(horizon, zenith, height);

    float aspect = max(uViewportAspect, 0.001);
    vec2 moonPos = vec2(0.78, 0.79);
    float moonDist = length((uv - moonPos) * vec2(aspect, 1.0));

    vec2 starGrid = vec2(118.0, 86.0);
    vec2 cell = floor(uv * starGrid);
    vec2 local = fract(uv * starGrid) - 0.5;
    float rnd = hash12(cell);
    float lowerSky = smoothstep(0.045, 0.18, uv.y);
    float moonClear = smoothstep(0.10, 0.18, moonDist);
    float starMask = step(0.982, rnd) * lowerSky * moonClear;
    float radius = mix(0.018, 0.052, hash12(cell + 19.7));
    float star = (1.0 - smoothstep(0.0, radius, length(local))) * starMask;
    vec3 starColor = mix(vec3(0.65, 0.74, 1.00), vec3(1.00, 0.90, 0.72), hash12(cell + 5.1));
    sky += starColor * star * mix(0.45, 1.15, hash12(cell + 11.3));

    float clouds = cloudMask(uv, aspect);
    sky = mix(sky, vec3(0.105, 0.120, 0.165), clouds);
    sky += vec3(0.018, 0.022, 0.032) * clouds;

    float moonGlow = 1.0 - smoothstep(0.055, 0.27, moonDist);
    sky += vec3(0.13, 0.15, 0.21) * moonGlow;

    float moonDisk = 1.0 - smoothstep(0.064, 0.074, moonDist);
    sky = mix(sky, vec3(0.88, 0.84, 0.68), moonDisk);

    return sky;
}

void main()
{
    if (uRenderMode == 2)
    {
        FragColor = vec4(renderNightSky(vTexCoord), 1.0);
        return;
    }

    if (uRenderMode == 1)
    {
        vec4 house = texture(uHouseTex, vTexCoord);
        if (house.a < 0.01)
            discard;
        vec3 nightHouse = house.rgb * vec3(0.22, 0.24, 0.34);
        FragColor = vec4(nightHouse, house.a);
        return;
    }

    float den = clamp(texture(uDensTex, vTexCoord).r, 0.0, 1.0);
    float alpha = smoothstep(0.035, 0.95, den);
    alpha = pow(alpha, 0.82);
    FragColor = vec4(vec3(0.92), alpha * 0.90);
}
