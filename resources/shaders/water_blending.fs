#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec2 TexCoords;

uniform sampler2D texture1;

void main()
{
    vec4 result = texture(texture1, TexCoords);
    result.w = 0.98;

    // distance darkening
    float dist = length(FragPos);

    result.x -= (result.x*dist)/40.0;
    result.y -= (result.y*dist)/40.0;
    result.z -= (result.z*dist)/40.0;

  //  result.x -= (result.x*pow(dist,1.75))/940.0;
  //  result.y -= (result.y*pow(dist,1.75))/940.0;
  //  result.z -= (result.z*pow(dist,1.75))/940.0;

    if(dist>40.0) result = vec4(0.0, 0.0, 0.0, 0.98);

    FragColor = vec4(result);
}