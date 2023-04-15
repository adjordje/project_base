#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float str;


void main() {
    vec3 crntPos = vec3(model * vec4(aPos + aNormal * str, 1.0));
    gl_Position = projection * view * vec4(crntPos, 1.0);
}